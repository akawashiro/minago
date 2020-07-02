//
// OBJ形式を扱う５
//   頂点座標、テクスチャ、法線ベクトルに対応
//   mtlファイルに対応
//

// TIPS:M_PIとかを使えるようにする
#define _USE_MATH_DEFINES

#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <netinet/in.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/stacktrace.hpp>

GLFWwindow *window;

// 画像を扱う
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "eyeLike.h"

// マテリアル
// TIPS メンバ変数にあらかじめ初期値を与えている
struct Material {
    GLfloat ambient[4] = {0.5f, 0.5f, 0.5f, 1.0f};
    GLfloat diffuse[4] = {0.5f, 0.5f, 0.5f, 1.0f};
    GLfloat specular[4] = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat shininess = 80.0f;
    GLfloat texture_id = 0;
};

// 読み込んだOBJ形式の情報
// NOTICE:OpenGLにそのまま渡してglDrawArraysで描画する
//        構造になっている
struct Mesh {
    std::vector<GLfloat> vtx;
    std::vector<GLfloat> texture;
    std::vector<GLfloat> normal;

    bool has_texture;
    bool has_normal;

    std::string mat_name;
};

struct Obj {
    std::vector<Mesh> mesh;
    std::map<std::string, Material> material;
};

// 面を構成する頂点情報
struct Face {
    int vi[3]; // 頂点番号
    int ti[3]; // テクスチャ座標
    int ni[3]; // 法線ベクトル番号
};

class LowPassFilter {
  public:
    void put(double x) { value = (1 - k) * value + k * x; }
    double get() { return value; }

  private:
    double value = 0;
    double k = 0.1;
};

class ObjLoader {
  public:
    // 画像を読み込む
    GLuint createTexture(std::string path) {
        // テクスチャを１つ生成
        GLuint id;
        glGenTextures(1, &id);

        // テクスチャを拘束
        // NOTICE
        // 以下テクスチャに対する命令は拘束したテクスチャに対して実行される
        glBindTexture(GL_TEXTURE_2D, id);

        // 画像を読み込む
        int width;
        int height;
        int comp;
        unsigned char *data = stbi_load((parent_directory + path).c_str(),
                                        &width, &height, &comp, 0);

        // アルファの有無でデータ形式が異なる
        GLint type = (comp == 3) ? GL_RGB : GL_RGBA;

        // 画像データをOpenGLへ転送
        glTexImage2D(GL_TEXTURE_2D, 0, type, width, height, 0, type,
                     GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);

        // 表示用の細々とした設定
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        return id;
    }

    // マテリアルを読み込む
    std::map<std::string, Material> loadMaterial(const std::string &path) {
        std::map<std::string, Material> material;

        std::ifstream stream(parent_directory + path);
        // ファイルを開けなければ、空の情報を返す
        if (!stream)
            return material;

        std::string cur_name;

        while (!stream.eof()) {
            std::string s;
            std::getline(stream, s);

            std::stringstream ss(s);
            std::string key;
            ss >> key;
            if (key == "newmtl") {
                ss >> cur_name;
            } else if (key == "Ns") {
                // スペキュラ指数
                ss >> material[cur_name].shininess;
            } else if (key == "Tr") {
                // 透過
            } else if (key == "Kd") {
                // 拡散光
                ss >> material[cur_name].diffuse[0] >>
                    material[cur_name].diffuse[1] >>
                    material[cur_name].diffuse[2];
            } else if (key == "Ks") {
                // スペキュラ
                ss >> material[cur_name].specular[0] >>
                    material[cur_name].specular[1] >>
                    material[cur_name].specular[2];
            } else if (key == "Ka") {
                // 環境光
                ss >> material[cur_name].ambient[0] >>
                    material[cur_name].ambient[1] >>
                    material[cur_name].ambient[2];
            } else if (key == "map_Kd") {
                // テクスチャ
                std::string tex_path;
                ss >> tex_path;

                // FIXME
                // 複数のマテリアルで同じテクスチャを参照している場合の最適化が必要
                material[cur_name].texture_id = createTexture(tex_path);
            }
        }

        return material;
    }

    // OBJ形式を読み込む
    Obj loadObj(const std::string &path) {
        auto last_slash = path.rfind("/", path.size() - 1);
        if (last_slash != std::string::npos) {
            parent_directory = path.substr(0, last_slash + 1);
        } else {
            parent_directory = "";
        }
        BOOST_LOG_TRIVIAL(info) << "parent_directory = " << parent_directory;

        std::ifstream stream(path);
        assert(stream);

        // ファイルから読み取った値を一時的に保持しておく変数
        std::vector<GLfloat> vtx;
        std::vector<GLfloat> normal;
        std::vector<GLfloat> texture;
        std::map<std::string, std::vector<Face>> face;
        std::string cur_mat(""); // マテリアル名

        // 変換後の値
        Obj obj;

        // NOTICE OBJ形式はデータの並びに決まりがないので、
        //        いったん全ての情報をファイルから読み取る
        while (!stream.eof()) {
            std::string s;
            std::getline(stream, s);

            // TIPS:文字列ストリームを使い
            //      文字列→数値のプログラム負荷を下げている
            std::stringstream ss(s);

            std::string key;
            ss >> key;
            if (key == "mtllib") {
                // マテリアル
                std::string m_path;
                ss >> m_path;
                obj.material = loadMaterial(m_path);
            } else if (key == "usemtl") {
                // 適用するマテリアルを変更
                ss >> cur_mat;
            } else if (key == "v") {
                // 頂点座標
                float x, y, z;
                ss >> x >> y >> z;
                vtx.push_back(x);
                vtx.push_back(y);
                vtx.push_back(z);
            } else if (key == "vt") {
                // 頂点テクスチャ座標
                float u, v;
                ss >> u >> v;
                texture.push_back(u);
                texture.push_back(v);
            } else if (key == "vn") {
                // 頂点法線ベクトル
                float x, y, z;
                ss >> x >> y >> z;
                normal.push_back(x);
                normal.push_back(y);
                normal.push_back(z);
            } else if (key == "f") {
                std::string text;
                std::vector<std::tuple<int, int, int>> vtns;
                while (ss >> text) {
                    // TIPS 初期値を-1にしておき、
                    //      「データが存在しない」状況に対応
                    int vi = -1, ti = -1, ni = -1;
                    std::stringstream fs(text);
                    {
                        std::string v;
                        std::getline(fs, v, '/');
                        vi = std::stoi(v) - 1;
                    }
                    {
                        std::string v;
                        std::getline(fs, v, '/');
                        if (!v.empty()) {
                            ti = std::stoi(v) - 1;
                        }
                    }
                    {
                        std::string v;
                        std::getline(fs, v, '/');
                        if (!v.empty()) {
                            ni = std::stoi(v) - 1;
                        }
                    }
                    vtns.push_back(std::make_tuple(vi, ti, ni));
                }
                for (int i = 2; i < vtns.size(); i++) {
                    Face f = {{std::get<0>(vtns[0]), std::get<0>(vtns[i - 1]),
                               std::get<0>(vtns[i])},
                              {std::get<1>(vtns[0]), std::get<1>(vtns[i - 1]),
                               std::get<1>(vtns[i])},
                              {std::get<2>(vtns[0]), std::get<2>(vtns[i - 1]),
                               std::get<2>(vtns[i])}};
                    face[cur_mat].push_back(f);
                }
            }
        }

        // 読み込んだ面情報から最終的な頂点配列を作成
        for (const auto &f : face) {
            Mesh mesh;
            // マテリアル名
            mesh.mat_name = f.first;

            mesh.has_texture = f.second[0].ti[0] >= 0;
            mesh.has_normal = f.second[0].ni[0] >= 0;

            // 頂点情報
            for (const auto &m : f.second) {
                for (int i = 0; i < 3; ++i) {
                    {
                        // 頂点座標
                        int index = m.vi[i] * 3;
                        for (int j = 0; j < 3; ++j) {
                            mesh.vtx.push_back(vtx[index + j]);
                        }
                    }
                    if (mesh.has_texture) {
                        // テクスチャ座標
                        int index = m.ti[i] * 2;
                        for (int j = 0; j < 2; ++j) {
                            mesh.texture.push_back(texture[index + j]);
                        }
                    }
                    if (mesh.has_normal) {
                        // 法線ベクトル
                        int index = m.ni[i] * 3;
                        for (int j = 0; j < 3; ++j) {
                            mesh.normal.push_back(normal[index + j]);
                        }
                    }
                }
            }

            obj.mesh.push_back(mesh);
        }

        // テクスチャ座標のY方向を反転
        for (auto &mesh : obj.mesh) {
            for (int i = 1; i < mesh.texture.size(); i += 2) {
                mesh.texture[i] = -mesh.texture[i];
            }
        }

        return obj;
    }

  private:
    std::string parent_directory;
};

// マテリアル設定
void setupMaterial(const Material &material) {
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, material.ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material.diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material.specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, material.shininess);

    // 自己発光
    GLfloat mat_emi[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, mat_emi);

    // テクスチャ
    glBindTexture(GL_TEXTURE_2D, material.texture_id);
    if (material.texture_id) {
        glEnable(GL_TEXTURE_2D);
    } else {
        // NOTICE 0番は「拘束なし」を意味する特別な値
        glDisable(GL_TEXTURE_2D);
    }
}

// メッシュ描画
void drawMesh(const Mesh &mesh) {
    glVertexPointer(3, GL_FLOAT, 0, &mesh.vtx[0]);
    glEnableClientState(GL_VERTEX_ARRAY);

    if (mesh.has_texture) {
        // NOTICE テクスチャ座標が有効かどうかで処理が変わる
        glTexCoordPointer(2, GL_FLOAT, 0, &mesh.texture[0]);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    } else {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    if (mesh.has_normal) {
        // NOTICE 法線ベクトルが有効かどうかで処理が変わる
        glNormalPointer(GL_FLOAT, 0, &mesh.normal[0]);
        glEnableClientState(GL_NORMAL_ARRAY);
    } else {
        glDisableClientState(GL_NORMAL_ARRAY);
    }

    glDrawArrays(GL_TRIANGLES, 0, mesh.vtx.size() / 3);
}

namespace obj_file_loader {

int run_main(std::string objfile_path, const double *const left_eye_center_x,
             const double *const right_eye_center_x,
             const double *const left_eye_center_y,
             const double *const right_eye_center_y) {
    // 初期化
    if (!glfwInit())
        return -1;

    // Window生成
    GLFWwindow *window =
        glfwCreateWindow(1600, 1200, "OBJ loader", nullptr, nullptr);

    if (!window) {
        // 生成失敗
        glfwTerminate();
        return -1;
    }

    // OpenGLの命令を使えるようにする
    glfwMakeContextCurrent(window);
    // アプリ画面更新タイミングをPCディスプレイに同期する
    glfwSwapInterval(1);

    // これ以降OpenGLの命令が使える

    ObjLoader obj_loader;
    Obj obj = obj_loader.loadObj(objfile_path);

    // 拡散光と鏡面反射を個別に計算する
    // TIPS:これで、テクスチャを張ったポリゴンもキラーン!!ってなる
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);

    // 透視変換行列を設定
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(35.0, 640 / 480.0, 0.2, 200.0);

    // 操作対象の行列をモデリングビュー行列に切り替えておく
    glMatrixMode(GL_MODELVIEW);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    // 並行光源の設定
    GLfloat diffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

    GLfloat ambient[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);

    GLfloat specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

    LowPassFilter x_lpf, y_lpf;

    while (glfwGetKey(window, GLFW_KEY_Q) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0) {
        double eyex = (*left_eye_center_x + *right_eye_center_x) / 2 - 0.5;
        double eyey = -(*left_eye_center_y + *right_eye_center_y) / 2 + 0.5;
        const double scale_x = 6.0;
        const double scale_y = scale_x / 9 * 16;
        eyex *= scale_x;
        eyey *= scale_y;
        // std::cout << "eyex = " << eyex << ", eyey = " << eyey << std::endl;
        x_lpf.put(eyex);
        y_lpf.put(eyey);

        // 単位行列を読み込む
        glLoadIdentity();
        glTranslatef(0, -0.7, 0.0);
        gluLookAt(x_lpf.get(), y_lpf.get(), 4, 0, 0, 0, 0, 1, 0);

        // 描画バッファの初期化
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ライトの設定
        GLfloat position[] = {0.0f, 0.0f, 4.0f, 0.0f};
        glLightfv(GL_LIGHT0, GL_POSITION, position);

        // 描画
        // TIPS マテリアルごとにグループ分けして描画
        for (const auto &mesh : obj.mesh) {
            setupMaterial(obj.material[mesh.mat_name]);
            drawMesh(mesh);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
}
} // namespace obj_file_loader
#include <fstream>
#include <boost/algorithm/string.hpp>
#include "db.hpp"
#include "httplib.h"

#define WWWROOT "./wwwroot"
// /video/**.mp4

using namespace httplib;
vod_system::TableVod *tb_video;

void VideoDelete(const Request &req, Response &rsp)
{
    //req.path = /video/1
    //1. 获取视频id
    int video_id = std::stoi(req.matches[1]);
    //2. 从数据库中获取到对应视频信息
    Json::Value json_rsp;
    Json::FastWriter writer;
    Json::Value video;
    bool ret = tb_video->GetOne(video_id, &video);
    if (ret == false) {
        std::cout << "mysql get video info failed!\n";
        rsp.status = 500;
        json_rsp["result"] = false;
        json_rsp["reason"] = "mysql get video info failed!";
        rsp.body = writer.write(json_rsp);
        rsp.set_header("Content-Type", "application/json");
        return ;
    }
    //3. 删除视频文件，封面图片文件 ./wwwroot + /video/**.mp4
    std::string vpath = WWWROOT + video["video_url"].asString();
    std::string ipath = WWWROOT + video["image_url"].asString();
    unlink(vpath.c_str());
    unlink(ipath.c_str());
    //4. 删除数据库中的数据
    ret = tb_video->Delete(video_id);
    if (ret == false) {
        rsp.status = 500;
        std::cout << "mysql delete video failed!\n";
        return ;
    }
    return ;
}
void VideoUpdate(const Request &req, Response &rsp)
{
    int video_id = std::stoi(req.matches[1]);
    Json::Value video;
    Json::Reader reader;
    //req.body = "{name:变形金刚, vdesc:好电影....}"
    bool ret = reader.parse(req.body, video);
    if (ret == false) {
        std::cout << "update video: parse video json failed!\n";
        rsp.status = 400;
        return;
    }
    ret = tb_video->Update(video_id, video);
    if (ret == false) {
        std::cout << "update video: mysql update failed!\n";
        rsp.status = 500;
        return;
    }
    return ;
}
void VideoGetAll(const Request &req, Response &rsp)
{
    Json::Value videos;
    Json::FastWriter writer;
    bool ret = tb_video->GetAll(&videos);
    if (ret == false) {
        std::cout << "getall video: mysql operation failed!\n";
        rsp.status = 500;
        return;
    }
    rsp.body = writer.write(videos);
    rsp.set_header("Content-Type", "application/json");
}
void VideoGetOne(const Request &req, Response &rsp)
{
    int video_id = std::stoi(req.matches[1]);
    Json::Value video;
    Json::FastWriter writer;
    bool ret = tb_video->GetOne(video_id, &video);
    if (ret == false) {
        std::cout << "getone video: mysql operation failed!\n";
        rsp.status = 500;
        return;
    }
    rsp.body = writer.write(video);
    rsp.set_header("Content-Type", "application/json");
}
#define VIDEO_PATH "/video/"
#define IMAGE_PATH "/image/"
void VideoUpload(const Request &req, Response &rsp)
{
    auto ret = req.has_file("video_name");
    if (ret == false) {
        std::cout << "have no video name!\n";
        rsp.status = 400;
        return;
    }   
    const auto& file = req.get_file_value("video_name");

    ret = req.has_file("video_desc");
    if (ret == false) {
        std::cout << "have no video desc!\n";
        rsp.status = 400;
        return;
    }   
    const auto& file1 = req.get_file_value("video_desc");

    ret = req.has_file("video_file");
    if (ret == false) {
        std::cout << "have no video file!\n";
        rsp.status = 400;
        return;
    }
    const auto& file2 = req.get_file_value("video_file");

    ret = req.has_file("image_file");
    if (ret == false) {
        std::cout << "have no image file!\n";
        rsp.status = 400;
        return;
    }
    const auto& file3 = req.get_file_value("image_file");
    const std::string &vname = file.content;//    变形金刚
    const std::string &vdesc = file1.content;//   刺激好看的电影
    const std::string &vfile = file2.filename;//  **.mp4
    const std::string &vcont = file2.content;//   content
    const std::string &ifile = file3.filename;//  **.jpg
    const std::string &icont = file3.content;//   content

    // ./wwwroot/video/aa.mp4    zhangsan873466346aa.mp4
    std::string vurl = VIDEO_PATH + file2.filename;
    std::string iurl = IMAGE_PATH + file3.filename;
    std::string wwwroot = WWWROOT;
    vod_system::Util::WriteFile(wwwroot + vurl, file2.content);
    vod_system::Util::WriteFile(wwwroot + iurl, file3.content);

    Json::Value video;
    video["name"] = vname;
    video["vdesc"] = vdesc;
    video["video_url"] = vurl;
    video["image_url"] = iurl;
    ret = tb_video->Insert(video);
    if (ret == false) {
        rsp.status = 500;
        std::cout << "insert video: mysql operation failed!\n";
        return;
    }
    rsp.set_redirect("/");
    return;
}

bool ReadFile(const std::string &name, std::string *body){
    std::ifstream ifile;
    ifile.open(name, std::ios::binary);
    if (!ifile.is_open()) {
        printf("open file failed:%s\n", name.c_str());
        ifile.close();
        return false;
    }
    ifile.seekg(0, std::ios::end);
    uint64_t length = ifile.tellg();
    ifile.seekg(0, std::ios::beg);
    body->resize(length);
    ifile.read(&(*body)[0], length);
    if (ifile.good() == false) {
        printf("read file failed:%s\n", name.c_str());
        ifile.close();
        return false;
    }
    ifile.close();
    return true;
}

void VideoPlay(const Request &req, Response &rsp)
{
    Json::Value video;
    int video_id = std::stoi(req.matches[1]);
    bool ret = tb_video->GetOne(video_id, &video);
    if (ret == false) {
        std::cout << "getone video: mysql operation failed!\n";
        rsp.status = 500;
        return;
    }
    std::string newstr =video["video_url"].asString();
    std::string oldstr = "{{video_url}}";
    std::string play_html = "./wwwroot/single-video.html";
    ReadFile(play_html, &rsp.body);
    boost::algorithm::replace_all(rsp.body, oldstr, newstr);
    rsp.set_header("Content-Type", "text/html");
    return ;
}
int main()
{
    tb_video = new vod_system::TableVod();
    Server srv;
    //正则表达式 \d-匹配一个数字字符； +匹配字符一次或多次
    // R"(string)" 括号中字符串中每个字符的特殊含义
    // /video/12  12
    srv.set_base_dir("./wwwroot"); //设置目录
    srv.Delete(R"(/video/(\d+))", VideoDelete);
    srv.Put(R"(/video/(\d+))", VideoUpdate);
    srv.Get(R"(/video)", VideoGetAll);
    srv.Get(R"(/video/(\d+))", VideoGetOne);
    srv.Post(R"(/video)", VideoUpload);
    srv.Get(R"(/play/(\d+))", VideoPlay);

    srv.listen("0.0.0.0", 9000);

    return 0;
}

#if 0
void test()
{
    vod_system::TableVod tb_vod;
    Json::Value val;
    //val["name"] = "电锯惊魂";
    //val["vdesc"] = "这是一个限制级的电影";
    //val["video_url"] = "/video/saw.mp4";
    //val["image_url"] = "/video/saw.jpg";
    //tb_vod.Insert(val);
    //tb_vod.Update(4, val);
    //tb_vod.Delete(4);
    tb_vod.GetOne(3, &val);
    Json::StyledWriter writer;
    std::cout << writer.write(val) << std::endl;
    return ;
}
#endif


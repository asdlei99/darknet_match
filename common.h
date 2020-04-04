#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <map>

#include <windows.h>
#include <process.h>
#include <stdio.h>

#include <d3d9.h>
#include <corecrt_io.h>
#pragma comment(lib,"d3d9.lib")

#include "list.h"
#include "data.h"
#include "parser.h"
#include "option_list.h"
#include "darknet.h"
#include "image.h"
#include "image_opencv.h"

#include <opencv2/opencv.hpp>

//默认字符串大小
constexpr int default_char_size = 1024;

//对象信息
struct object_info
{
	int left, right, top, down;//位置信息
	int class_index;//所属类
	float confidence;//置信度
};

//颜色信息
struct color_info
{
	//方框颜色
	float box_rgb[3];

	//字体颜色
	float font_rgb[3];

	//厚度
	float thickness;

	color_info() : thickness(1.0f) {}
};

//分析相关
struct set_detect_info
{
	double detect_time;//检测耗时

	float thresh;//阈值
	float hier_thresh;//阈值
	float nms;//非极大值抑制

	set_detect_info() :thresh(0.25f), hier_thresh(0.5f), nms(0.45f) {}
};

//视频控制相关
struct video_control
{
	//视频路径
	char video_path[default_char_size];

	//是否退出线程
	bool leave;

	//检测线程数量
	int detect_count;

	//显示延迟 读取延迟 检测延迟
	int show_delay, read_delay, detect_delay;

	video_control() :leave(true), detect_count(4) 
	{
		show_delay = read_delay = detect_delay = 10;
	}
};

//视频帧相关
struct video_frame_info
{
	//是否正在预测
	bool detecting;

	//是否能够显示
	bool display;

	//原始视频帧
	cv::Mat original_frame;
};

//视频处理相关
struct video_handle_info
{
	//是否结束线程
	bool break_state;

	//相关线程是否还在工作
	bool read_frame, detect_frame;

	//视频类指针
	cv::VideoCapture cap;

	//队列最多数量,因为读取速度远远快于检测速度
	int max_frame_count;

	//检测视频帧队列
	std::vector<video_frame_info*> detect_datas;

	//关键段用于线程同步
	CRITICAL_SECTION critical_srction;

	//各线程延迟设置
	int *show_delay, *read_delay, *detect_delay;

	void initialize()
	{ 
		break_state = false;
		read_frame = detect_frame = true;
		max_frame_count = 50;
		show_delay = read_delay = detect_delay = nullptr;
		InitializeCriticalSection(&critical_srction); 
	}
	void entry() { EnterCriticalSection(&critical_srction); }
	void leave() { LeaveCriticalSection(&critical_srction); }
	void clear() 
	{ 
		break_state = true;
		read_frame = detect_frame = false;
		if (cap.isOpened()) cap.release();
		for (auto& it : detect_datas) delete it;
		detect_datas.clear();
		DeleteCriticalSection(&critical_srction); 
	}
};

//网络层
struct net_info_set
{
	bool initizlie;//是否初始化了
	int classes;//类别数量

	char** classes_name;//标签名字
	struct network match_net;//网络结构
};

//界面显示信息
struct imgui_set
{
	//文件配置窗口
	bool show_file_set_window;

	//测试图片窗口
	bool show_test_picture_window;

	//测试视频窗口
	bool show_test_video_window;

	//测试摄像头窗口
	bool show_test_camera_window;

};

//全局设置信息
struct global_set
{
	//窗口类名称
	char window_class_name[1024];

	//窗口句柄
	HWND window_hwnd;

	//d3d9设备相关
	LPDIRECT3D9 direct3d9;
	D3DPRESENT_PARAMETERS d3dpresent;
	LPDIRECT3DDEVICE9 direct3ddevice9;
	LPDIRECT3DTEXTURE9 direct3dtexture9;

	//图像数据，用于在imgui中显示图片
	int width, height, channel;
	unsigned char* picture_data;

	//界面显示相关
	struct imgui_set imgui_show_set;

	//网络层相关
	struct net_info_set net_set;

	//颜色相关
	struct color_info color_set;

	//视频检测相关
	struct set_detect_info video_detect_set;
};
extern global_set g_global_set;

//严重错误检查
void check_serious_error(bool state, const char* show_str = "");

//显示窗口提示
void show_window_tip(const char* str);

//获取显卡的数量
int get_gpu_count();

//获取显卡相关信息
cudaDeviceProp* get_gpu_infomation(int gpu_count);

//获取系统类型
int get_os_type();

//获取CPU核心数
int get_cpu_kernel();

//获取物理内存总数
int get_physical_memory();

//选择指定类型的一个文件
bool select_type_file(const char* type_file, char* return_str, int return_str_size = default_char_size);

//读取标签名称
void read_classes_name(std::vector<std::string>& return_data, const char* path);

//初始化网络
bool initialize_net(const char* names_file, const char* cfg_file, const char* weights_file);

//清理网络
void clear_net();

//加载图片数据
void read_picture_data(const char* target, image& picture_data, cv::Mat& opencv_data, cv::Mat& rgb_data);

//将Mat数据转化为image数据
void mat_translate_image(const cv::Mat& opencv_data, image& image_data);

//预测图片
void analyse_picture(const char* target, set_detect_info& detect_info, bool show = false);

//绘制方框和类型
void draw_boxs_and_classes(cv::Mat& picture_data, box box_info, const char* name);

//计算对象位置信息
void get_object_rect(int width, int height, box& box_info, object_info& object);

//绘制方框
void draw_object_rect(cv::Mat& buffer, int left, int top, int right, int down);

//将字符串转化为utf8编码
std::string string_to_utf8(const char* str);

//更新置图片纹理数据
void update_picture_texture(cv::Mat& opencv_data);

//分析视频文件
unsigned __stdcall analyse_video(void* prt);

//读取视频帧线程
unsigned __stdcall read_frame_proc(void* prt);

//预测视频帧对象线程
unsigned __stdcall prediction_frame_proc(void* prt);








std::vector<std::string> get_path_from_str(const char* str, const char* file_type);
void picture_to_label(const char* path, std::map<std::string,int>& class_names);




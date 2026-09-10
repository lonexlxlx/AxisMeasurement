#pragma once
#include <iostream>
#include <numeric>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <QProcess>

using namespace std;
class roughnessFun
{
public:
	vector<float> MaxMinNormalization(const vector<float>& arr);
	float Laplacian(const cv::Mat& img);
	pair<float, string> find_max_value_and_filename(const vector<float>& arr, const vector<string>& filenames);
	pair<float, string> processFolder(const string& img_base_path);
	double rougthnessCaculate(QString imagePath);

};

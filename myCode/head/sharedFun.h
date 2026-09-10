//各检测程序公用的一些函数
#pragma once
#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <QString>
#include <HalconCpp.h>
#include <Halcon.h>
#include "HDevThread.h"

using namespace std;
namespace fs = std::filesystem;
void deleteFolderContents(const fs::path& folderPath, const std::vector<std::string>& allowedExtensions);//用于清空文件夹中指定格式的文件
QString addSymbol(QString number); 
QString runtimePath(const QString& relativePath);
bool dataScreening_minTwo(vector<double>& orgResult, vector<double>& processedResult);
bool dataScreening_maxTwo(vector<double>& orgResult, vector<double>& processedResult);
bool dataScreening_middleTwo(vector<double>& orgResult, vector<double>& processedResult);
//误差补偿相关函数***
double axis5_compensation(double axis5_encodePosition);
double diameter_compensation(float diameter);




#include "roughnessFun.h"

vector<float> roughnessFun::MaxMinNormalization(const vector<float>& arr) {
	vector<float> out_arr;
	float min_val = *min_element(arr.begin(), arr.end());
	float max_val = *max_element(arr.begin(), arr.end());
	for (const auto& x : arr) {
		float normalized_val = (x - min_val) / (max_val - min_val);
		out_arr.push_back(normalized_val);
	}
	return out_arr;
};
float roughnessFun::Laplacian(const cv::Mat& img) {
	cv::Mat laplacian_img;
	cv::Laplacian(img, laplacian_img, CV_64F);
	cv::Scalar mean, stddev;
	cv::meanStdDev(laplacian_img, mean, stddev);
	float variance = stddev.val[0] * stddev.val[0];
	return variance;
};
pair<float, string> roughnessFun::find_max_value_and_filename(const vector<float>& arr, const vector<string>& filenames) {
	if (arr.empty()) {
		return make_pair(-1.0f, "");
	}

	auto max_value_iter = max_element(arr.begin(), arr.end());
	int max_index = distance(arr.begin(), max_value_iter);
	string max_filename = filenames[max_index];

	return make_pair(*max_value_iter, max_filename);
};
pair<float, string> roughnessFun::processFolder(const string& img_base_path) {
	vector<float> Laplacian_arr_sum;
	vector<string> path_list;
	vector<vector<int>> gaussian = {
		{1, 2, 8, 2, 1},
		{2, 4, 8, 4, 2},
		{2, 4, 16, 4, 2},
		{2, 4, 8, 4, 2},
		{1, 2, 8, 2, 1}
	};

	int gaussian_sum = accumulate(gaussian.begin(), gaussian.end(), 0,
		[](int sum, const vector<int>& row) {
			return sum + accumulate(row.begin(), row.end(), 0);
		});

	for (const auto& entry : filesystem::directory_iterator(img_base_path)) {
		path_list.push_back(entry.path().string());
	}

	sort(path_list.begin(), path_list.end(), [](const string& a, const string& b) {
		string a_filename = a.substr(a.find_last_of("\\/") + 1);
		string b_filename = b.substr(b.find_last_of("\\/") + 1);
		return stoi(a_filename) < stoi(b_filename);
		});

	for (const auto& filename : path_list) {
		//cout << filename << endl;
		cv::Mat img = cv::imread(filename, cv::IMREAD_GRAYSCALE);
		int padding_height = (520 - 512) / 2;
		int padding_width = (520 - 512) / 2;
		cv::Mat padding_img;
		cv::copyMakeBorder(img, padding_img, padding_height, padding_height, padding_width, padding_width, cv::BORDER_CONSTANT, cv::Scalar(0));

		int small_img_size = 104;
		int rows = 5;
		int cols = 5;
		vector<cv::Mat> small_imgs(rows * cols, cv::Mat(small_img_size, small_img_size, CV_8UC1));

		for (int i = 0; i < rows; ++i) {
			for (int j = 0; j < cols; ++j) {
				cv::Rect roi(j * small_img_size, i * small_img_size, small_img_size, small_img_size);
				cv::Mat small_img = padding_img(roi);
				small_img.copyTo(small_imgs[i * cols + j]);
			}
		}

		vector<float> sharpness(rows * cols, 0.0f);
		for (int i = 0; i < rows; ++i) {
			for (int j = 0; j < cols; ++j) {
				cv::Mat small_img = small_imgs[i * cols + j];
				sharpness[i * cols + j] = Laplacian(small_img);
				if (i + j == rows + cols - 2) {
					float weighted_sharpness = 0.0f;
					for (int r = 0; r < rows; ++r) {
						for (int c = 0; c < cols; ++c) {
							weighted_sharpness += gaussian[r][c] * sharpness[r * cols + c];
						}
					}
					weighted_sharpness /= gaussian_sum;
					Laplacian_arr_sum.push_back(weighted_sharpness);
				}
			}
		}
	}

	vector<float> Laplacian_arr_normal = MaxMinNormalization(Laplacian_arr_sum);
	pair<float, string> max_value_filename = find_max_value_and_filename(Laplacian_arr_normal, path_list);

	return max_value_filename;
};
double roughnessFun::rougthnessCaculate(QString imagePath)
{
	// 创建QProcess对象来运行Python脚本
	QProcess process;

	// 设置要执行的命令（这里假设您的Python环境位于 "E:/anaconda/envs/py37/python.exe"）
	//QString pythonInterpreter = "C:/Users/PC/.conda/envs/mytf/python.exe";
	QString pythonInterpreter = "C:/ProgramData/Anaconda3/envs/331py37/python.exe";
	QString pythonScript = "./programParmeter/roughness/331roughness.py";

	// 图像路径和模型路径 
	QString modelPath = "./programParmeter/roughness/model_best_weights.h5";
	// 设置Python脚本的参数
	QStringList arguments;
	arguments << imagePath << modelPath;

	// 设置QProcess的相关属性
	process.setReadChannel(QProcess::StandardOutput);  // 捕获标准输出
	process.start(pythonInterpreter, QStringList() << pythonScript << arguments);
	process.waitForFinished();

	// 读取Python脚本的输出
	QByteArray output = process.readAllStandardOutput();
	QString outputString(output);

	// 从输出中提取预测结果
	QString predictionStr = outputString.trimmed();
	bool ok;
	double prediction = predictionStr.toDouble(&ok);//最终粗糙度预测结果
	cout << prediction << endl;
	if (ok) {
		// 输出预测结果的数字部分
		cout << "Prediction:" << prediction;
	}
	else {
		cout << "Failed to convert prediction to double";
	}

	return prediction;
};

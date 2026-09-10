#include "sharedFun.h"
#include <QCoreApplication>
#include <QDir>

/*
函数功能：删除文件夹下所有指定格式的文件，以及空文件夹
参数说明：folderPath是目标文件夹地址；allowedExtensions是要删除的文件后缀名所储存的vectors数组
*/
void deleteFolderContents(const fs::path& folderPath, const std::vector<std::string>& allowedExtensions) {
    for (const auto& entry : fs::directory_iterator(folderPath)) {
        //cout << entry.path() ;
        if (entry.is_directory()) {
            //cout << "is_directory" << endl;
            
            deleteFolderContents(entry.path(), allowedExtensions);
            if (fs::is_empty(entry.path()))
            {
                fs::remove(entry.path());
            };
        }
        else if (entry.is_regular_file()) {
            if (std::find(allowedExtensions.begin(), allowedExtensions.end(), entry.path().extension()) != allowedExtensions.end()) {
                fs::remove(entry.path());
            }
        }
    }
};
QString runtimePath(const QString& relativePath)
{
    return QDir(QCoreApplication::applicationDirPath())
        .filePath(relativePath);
}
/*
函数功能：给QString类型的函数添加正负号
*/
QString addSymbol(QString number)
{
    float value=number.toFloat();
    if (value > 0)
    {
        return "+" + number;
    }
    else
    {
        return number;
    }
};

bool dataScreening_minTwo(vector<double>& orgResult, vector<double>& processedResult)//dui
{
    int orgDataLength = orgResult.size();
    if (orgDataLength >= 2)
    {
        /*
        cout << "原始数据:" << endl;
        for (int i = 0; i < orgDataLength; i++)
        {
            cout << orgResult[i] << "   ";
        };
        cout << endl;
        */
        sort(orgResult.begin(), orgResult.end());
       
        cout << "保留的最小数据" << orgResult[0] << " - " << orgResult[1] << endl;
        processedResult.push_back(orgResult[0]);
        processedResult.push_back(orgResult[1]); 
        return true;
    }
    else if (orgDataLength == 1)
    {
        processedResult.push_back(orgResult[0]);
        return true;
    }
    else {

        return false;
    }
};
bool dataScreening_maxTwo(vector<double>& orgResult, vector<double>& processedResult)//dui
{
    int orgDataLength = orgResult.size();
    if (orgDataLength == 1)
    {
        if (orgResult[orgDataLength - 1] >= 2000)
        {
            processedResult.push_back(9999);//如果超上限则写入9999
            return true;
        }
        else 
        {
            processedResult.push_back(orgResult[orgDataLength - 1]);
            return true;
        };
    }
    if (orgDataLength > 1)
    {
        sort(orgResult.begin(), orgResult.end());
        if (orgResult[orgDataLength - 1] >= 2000)//如果最大值超过上限踢出掉该值后重新排序
        {
            orgResult.pop_back();
            bool maxTwoFlag = dataScreening_maxTwo(orgResult, processedResult);
            return maxTwoFlag;
        }
        else
        {
            orgDataLength = orgResult.size();
            cout << "保留的最大数据" << orgResult[orgDataLength - 1] << "  -" << orgResult[orgDataLength - 2] << endl;
            processedResult.push_back(orgResult[orgDataLength - 1]);
            processedResult.push_back(orgResult[orgDataLength - 2]);
            return true;
        }
    }
    else
    {
        return false;
    };
};
bool dataScreening_middleTwo(vector<double>& orgResult, vector<double>& processedResult)
{
    int orgDataLength = orgResult.size();
    int halfIndex = orgDataLength / 2;
    if (orgDataLength =1)
    {
        processedResult.push_back(orgResult[0]);
        return true;
    }
    else if(orgDataLength >= 2)
    {
        /*
        cout << "原始数据:" << endl;
        for (int i = 0; i < orgDataLength; i++)
        {
            cout << orgResult[i] << "   ";
        };
        cout << endl;
        */
        sort(orgResult.begin(), orgResult.end());

        cout << "保留的中间值数据" << orgResult[halfIndex - 1] << "  -" << orgResult[halfIndex] << endl;
        processedResult.push_back(orgResult[halfIndex - 1]);
        processedResult.push_back(orgResult[halfIndex]);
        return true;
    }
    else {

        return false;
    }
};
double axis5_compensation(double axis5_encodePosition)
{
    double axis5_realReference = axis5_encodePosition * 0.0005;
    double axis5_real;
    if (0 <= axis5_realReference && axis5_realReference < 320)
    {
        axis5_real = axis5_realReference - ((1.82671795108333e-06) * axis5_realReference * axis5_realReference + (-0.000658181561431101) * axis5_realReference
            + 0.0354087938903302);
    }
    else if (320 <= axis5_realReference && axis5_realReference < 960)
    {
        axis5_real = axis5_realReference - ((-7.23209761037024e-10) * axis5_realReference * axis5_realReference * axis5_realReference + (1.49651432096357e-06) * axis5_realReference * axis5_realReference
            - 0.000987356680310842 * axis5_realReference + 0.207519788734567);
    }
    else if (960 <= axis5_realReference && axis5_realReference < 1400)
    {
        axis5_real = axis5_realReference - ((1.03211182532580e-08) * axis5_realReference * axis5_realReference * axis5_realReference + (-3.67822483559402e-05) * axis5_realReference * axis5_realReference
            + 0.0432501678284173 * axis5_realReference -16.7778017380817);
    }
   
    else if (1400 <= axis5_realReference && axis5_realReference < 1600)
    {
        axis5_real = axis5_realReference - ((4.66843760059769e-09) * axis5_realReference * axis5_realReference * axis5_realReference + (-2.15355753242042e-05) * axis5_realReference * axis5_realReference
            + 0.0330763951255681 * axis5_realReference - 16.9040836936982);
    }
    else
    {
        axis5_real = axis5_realReference;
    };
    return axis5_real;
};
double diameter_compensation(float diameter)
{
    double diameter1 = diameter;
    double diameterReal;
    diameterReal = diameter1 -((4.98050682261207e-08) * diameter1 * diameter1* diameter1 +(-1.31532163742690e-05) * diameter1 * diameter1+ (0.000985510721247562) * diameter1
         -0.0208181286549708);
    return diameterReal;
};

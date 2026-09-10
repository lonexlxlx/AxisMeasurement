#include "program0.h"

program_0::program_0(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

	SetConsoleOutputCP(65001);

    //测量数据区设置
    //设置表格控件的表头
    //ui.measureTable->setRowCount(200);    //设置行数
	measureTablePtr = ui.measureTable;
    ui.measureTable->setColumnCount(9); //设置列数
    ui.measureTable->setEditTriggers(QAbstractItemView::NoEditTriggers); //禁止编辑
    ui.measureTable->horizontalHeader()->setHighlightSections(false);//点击表时不对表头行光亮（获取焦点）
    QFont font = ui.measureTable->horizontalHeader()->font();
    font.setBold(true);//表头字体加粗
    ui.measureTable->horizontalHeader()->setFont(font);
    ui.measureTable->horizontalHeader()->setStyleSheet("QHeaderView::section{background:lightgray;}"); //skyblue设置表头背景色
    ui.measureTable->setStyleSheet("selection-background-color:lightblue;"); //设置选中背景色
    ui.measureTable->setFont(QFont("song", 11));//设置表格字体  
    QStringList header;
	header << "特征号" << "特征名称" << "测量结果" << "最小值" << "最大值" << "次数" << "公称值" << "下限值" << "上限值";
	ui.measureTable->setHorizontalHeaderLabels(header);//设置表头（横）
    ui.measureTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); //表列随着表格变化而自适应变化
    ui.measureTable->show();

    //视觉检测算法参数初始化
    ImageRowSize_Cali = ImageRowSize * Calik;//图像换算后行距离
    ImageColSize_Cali = ImageColSize * Calik;//图像换算后列距离
}

program_0::~program_0()
{}

void program_0::pushback_vectors(vector<double>& fResult, int fType, double fIndex, double fNominalsize, double fLowerTolerance, double fUpperTolerance)
{
	//将对应测量结果写入动态数组储存
	double fLowerSize = fNominalsize + fLowerTolerance;//下公差尺寸
	double fUpperSize = fNominalsize + fUpperTolerance;//上公差尺寸
	int resultLength = fResult.size();
	double minResult = *min_element(fResult.begin(), fResult.end());
	double maxResult = *max_element(fResult.begin(), fResult.end());
	double fQualify[3];//0号是min的判断结果，1号是max的判断结果；2号是综合判断结果

	if (minResult >= fLowerSize && minResult <= fUpperSize) {//特征检出尺寸是否位于合格尺寸内。是否合格0合格1不合格
		fQualify[0] = 0;
		fQualify[2] = 0;
	}
	else {
		fQualify[0] = 1;
		fQualify[2] = 1;
	};
	if (maxResult >= fLowerSize && maxResult <= fUpperSize) {//特征检出尺寸是否位于合格尺寸内。是否合格0合格1不合格
		fQualify[1] = 0;
		fQualify[2] = 0;
	}
	else {
		fQualify[1] = 1;
		fQualify[2] = 1;
	};
	if (fQualify[2])
	{
		//allFeatureFlag = "NG";
		//ngFeatureNum++;
	}
	minResult = round(minResult * 10000) / 10000;//元整保留4位小数
	maxResult = round(maxResult * 10000) / 10000;
	/*
	vector<double> temp_fGlobal;
	temp_fGlobal.push_back(fIndex);
	temp_fGlobal.push_back(fType);
	temp_fGlobal.push_back(fQualify);
	temp_fGlobal.push_back();
	temp_fGlobal.push_back(fNominalsize);
	temp_fGlobal.push_back(fLowerTolerance);
	temp_fGlobal.push_back(fUpperTolerance);
	fGlobal.push_back(temp_fGlobal);
	*/

	//将测量结果显示在桌面表格上
	int row = measureTablePtr->rowCount();
	measureTablePtr->insertRow(row);//增加一行
	QTableWidgetItem* item[9];
	for (int i = 0; i < 9; i++)
	{
		item[i] = new QTableWidgetItem("");
		item[i]->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);//居中设置（有可能可以不用）
	};
	item[0]->setText(QString::number(fIndex));//第一列特征编号
	measureTablePtr->setItem(row, 0, item[0]);
	item[1]->setText(featureMap[fType]);//显示特征名称
	measureTablePtr->setItem(row, 1, item[1]);
	item[2]->setText(featureQualityMap[fQualify[2]]);//显示测量判断结果
	if (fQualify[2])
	{
		item[2]->setBackground(QColor(230, 0, 0));
	}
	else
	{
		item[2]->setBackground(QColor(0, 200, 0));
	};
	measureTablePtr->setItem(row, 2, item[2]);
	item[3]->setText(QString::number(minResult));//显示测量最小值
	if (fQualify[0])
	{
		item[3]->setBackground(QColor(230, 0, 0));
	}
	else
	{
		item[3]->setBackground(QColor(0, 200, 0));
	};
	measureTablePtr->setItem(row, 3, item[3]);
	item[4]->setText(QString::number(maxResult));//显示测量最大值
	if (fQualify[1])
	{
		item[4]->setBackground(QColor(230, 0, 0));
	}
	else
	{
		item[4]->setBackground(QColor(0, 200, 0));
	};
	measureTablePtr->setItem(row, 4, item[4]);
	item[5]->setText(QString::number(resultLength));//显示测量次数
	measureTablePtr->setItem(row, 5, item[5]);
	item[6]->setText(QString::number(fNominalsize));//显示公称值
	measureTablePtr->setItem(row, 6, item[6]);
	item[7]->setText(QString::number(fLowerTolerance));//显示下限值
	measureTablePtr->setItem(row, 7, item[7]);
	item[8]->setText(QString::number(fUpperTolerance));//显示上限值
	measureTablePtr->setItem(row, 8, item[8]);
	fResult.clear();
	fResult.shrink_to_fit();
};




//以下是调用测试
//卡尺直接测量
//函数中直接传图像
void program_0::Straight_TLineFP_P(double* TLineFP_result, HObject ho_Image, int Line1, int Line2, int Line3, int Line4,
	int Metrology1, int Metrology2, int Metrology3, int Metrology4)
{
	// Local iconic variables
	HObject  ho_LineContours, ho_LineContour;
	// Local control variables
	HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices;
	HTuple  hv_RowLine, hv_ColumnLine, hv_RowBegin, hv_ColBegin;
	HTuple  hv_RowEnd, hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;
	HTuple  hv_Width, hv_Height;
	GetImageSize(ho_Image, &hv_Width, &hv_Height);


	CreateMetrologyModel(&hv_MetrologyHandle);

	hv_Line.Clear();
	hv_Line[0] = Line1;
	hv_Line[1] = Line2;
	hv_Line[2] = Line3;
	hv_Line[3] = Line4;
	AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, Metrology1, Metrology2, Metrology3, Metrology4,
		HTuple(), HTuple(), &hv_LineIndices);
	SetMetrologyObjectParam(hv_MetrologyHandle, "all", "measure_distance", 0.3);
	ApplyMetrologyModel(ho_Image, hv_MetrologyHandle);

	GetMetrologyObjectMeasures(&ho_LineContours, hv_MetrologyHandle, "all", "all",
		&hv_RowLine, &hv_ColumnLine);
	if (HDevWindowStack::IsOpen())
		SetColor(HDevWindowStack::GetActive(), "yellow");
	if (HDevWindowStack::IsOpen())
		SetLineWidth(HDevWindowStack::GetActive(), 2);
	if (HDevWindowStack::IsOpen())
		DispObj(ho_LineContours, HDevWindowStack::GetActive());
	GetMetrologyObjectResultContour(&ho_LineContour, hv_MetrologyHandle, 0, "all",
		1.5);
	//get_metrology_object_model_contour (ModelContour, MetrologyHandle, 'all', 1.5)
	if (HDevWindowStack::IsOpen())
		SetColor(HDevWindowStack::GetActive(), "blue");
	if (HDevWindowStack::IsOpen())
		SetLineWidth(HDevWindowStack::GetActive(), 2);
	if (HDevWindowStack::IsOpen())
		DispObj(ho_LineContour, HDevWindowStack::GetActive());
	FitLineContourXld(ho_LineContour, "tukey", -1, 0, 5, 2, &hv_RowBegin, &hv_ColBegin,
		&hv_RowEnd, &hv_ColEnd, &hv_Nr, &hv_Nc, &hv_Dist);
	cout << " hv_result():" << hv_RowBegin[0].D() << "  " << hv_ColBegin[0].D() << "  " << hv_RowEnd[0].D() << "  " << hv_ColEnd[0].D() << endl;
	TLineFP_result[0] = hv_RowBegin[0].D();
	TLineFP_result[1] = hv_ColBegin[0].D();
	TLineFP_result[2] = hv_RowEnd[0].D();
	TLineFP_result[3] = hv_ColEnd[0].D();
}

void program_0::Straight_TLineFP_P137(double* TLineFP_result, HObject ho_Image, int Line1, int Line2, int Line3, int Line4,
	int Metrology1, int Metrology2, int Metrology3, int Metrology4)
{
	// Local iconic variables
	HObject  ho_LineContours, ho_LineContour;
	// Local control variables
	HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices;
	HTuple  hv_RowLine, hv_ColumnLine, hv_RowBegin, hv_ColBegin;
	HTuple  hv_RowEnd, hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;
	HTuple  hv_Width, hv_Height;
	HObject  ho_ImageMean1, ho_ImageEmphasize1, ho_ImageIlluminate, ho_ImageEquHisto;
	GetImageSize(ho_Image, &hv_Width, &hv_Height);


	CreateMetrologyModel(&hv_MetrologyHandle);

	MeanImage(ho_Image, &ho_ImageMean1, 9, 9);
	//图像增强
	Emphasize(ho_ImageMean1, &ho_ImageEmphasize1, 3, 3, 0.9);
	//照射增强对比。图像中非常暗的部分被强烈“照亮”，非常亮的部分被“暗化”
	Illuminate(ho_ImageEmphasize1, &ho_ImageIlluminate, 10, 190, 0.75);
	EquHistoImage(ho_ImageIlluminate, &ho_ImageEquHisto);

	hv_Line.Clear();
	hv_Line[0] = Line1;
	hv_Line[1] = Line2;
	hv_Line[2] = Line3;
	hv_Line[3] = Line4;
	AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, Metrology1, Metrology2, Metrology3, Metrology4,
		HTuple(), HTuple(), &hv_LineIndices);
	SetMetrologyObjectParam(hv_MetrologyHandle, "all", "measure_distance", 0.3);
	ApplyMetrologyModel(ho_ImageEquHisto, hv_MetrologyHandle);

	GetMetrologyObjectMeasures(&ho_LineContours, hv_MetrologyHandle, "all", "all",
		&hv_RowLine, &hv_ColumnLine);
	if (HDevWindowStack::IsOpen())
		SetColor(HDevWindowStack::GetActive(), "yellow");
	if (HDevWindowStack::IsOpen())
		SetLineWidth(HDevWindowStack::GetActive(), 2);
	if (HDevWindowStack::IsOpen())
		DispObj(ho_LineContours, HDevWindowStack::GetActive());
	GetMetrologyObjectResultContour(&ho_LineContour, hv_MetrologyHandle, 0, "all",
		1.5);
	//get_metrology_object_model_contour (ModelContour, MetrologyHandle, 'all', 1.5)
	if (HDevWindowStack::IsOpen())
		SetColor(HDevWindowStack::GetActive(), "blue");
	if (HDevWindowStack::IsOpen())
		SetLineWidth(HDevWindowStack::GetActive(), 2);
	if (HDevWindowStack::IsOpen())
		DispObj(ho_LineContour, HDevWindowStack::GetActive());
	FitLineContourXld(ho_LineContour, "tukey", -1, 0, 5, 2, &hv_RowBegin, &hv_ColBegin,
		&hv_RowEnd, &hv_ColEnd, &hv_Nr, &hv_Nc, &hv_Dist);
	cout << " hv_result():" << hv_RowBegin[0].D() << "  " << hv_ColBegin[0].D() << "  " << hv_RowEnd[0].D() << "  " << hv_ColEnd[0].D() << endl;
	TLineFP_result[0] = hv_RowBegin[0].D();
	TLineFP_result[1] = hv_ColBegin[0].D();
	TLineFP_result[2] = hv_RowEnd[0].D();
	TLineFP_result[3] = hv_ColEnd[0].D();
}


//模板匹配找特征点
void program_0::TemplateMatching_TLineFP_P(double* TLineFP_result, HObject ho_Image, HTuple hv_ModelFile, int MatchingLine1, int MatchingLine2, int MatchingLine3, int MatchingLine4,
	int Metrology1, int Metrology2, int Metrology3, int Metrology4,
	double ShapeModel1, double ShapeModel2, double ShapeModel3, double ShapeModel4, double ShapeModel5) {
	// Local iconic variables
	HObject  ho_ModelContours, ho_LineContours;
	HObject  ho_LineContour;

	// Local control variables
	HTuple  hv_Width, hv_Height, hv_ReusedModelID;
	HTuple  hv_MetrologyHandle, hv_Line, hv_LineIndices, hv_ReusedRefPointRow;
	HTuple  hv_ReusedRefPointCol, hv_NumLevels, hv_AngleStart;
	HTuple  hv_AngleExtent, hv_AngleStep, hv_ScaleMin, hv_ScaleMax;
	HTuple  hv_ScaleStep, hv_Metric, hv_MinContrast, hv_Row3;
	HTuple  hv_Column3, hv_Angle, hv_Score, hv_HomMat2D, hv_RowLine;
	HTuple  hv_ColumnLine, hv_RowBegin, hv_ColBegin, hv_RowEnd;
	HTuple  hv_ColEnd, hv_Nr, hv_Nc, hv_Dist;

	//读取模板和图像进行模板匹配*
	GetImageSize(ho_Image, &hv_Width, &hv_Height);

	//hv_ModelFile = "./programParmeter/program0_1080new/cam0_10-460-209.sbm";
	ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
	//创建测量模型
	CreateMetrologyModel(&hv_MetrologyHandle);
	//添加直线测量工具 这里的卡尺参数要根据模板的长度进行修改
	hv_Line.Clear();
	hv_Line[0] = MatchingLine1;
	hv_Line[1] = MatchingLine2;
	hv_Line[2] = MatchingLine3;
	hv_Line[3] = MatchingLine4;

	//AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, 50, 12, 1, 70,
	//	HTuple(), HTuple(), &hv_LineIndices);
	AddMetrologyObjectGeneric(hv_MetrologyHandle, "line", hv_Line, Metrology1, Metrology2, Metrology3, Metrology4,
		HTuple(), HTuple(), &hv_LineIndices);
	SetMetrologyObjectParam(hv_MetrologyHandle, "all", "measure_distance", 0.3);
	//获取读取的模板的轮廓，区域坐标等信息
	GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
	GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
	GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
		&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
	//进行模板匹配
	//FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.2,
	//	1, 0.3, "least_squares", 0, 0.7, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
	FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, ShapeModel1,
		ShapeModel2, ShapeModel3, "least_squares", ShapeModel4, ShapeModel5, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
	//显示形状特征模板匹配结果


	VectorAngleToRigid(hv_ReusedRefPointRow, hv_ReusedRefPointCol, 0, hv_Row3, hv_Column3,
		hv_Angle, &hv_HomMat2D);
	//affine_trans_contour_xld (ModelContours, ContoursAffinTrans, HomMat2D)
	//dev_set_color ('green')
	//dev_display (ContoursAffinTrans)
	//对测量模型执行变换
	AlignMetrologyModel(hv_MetrologyHandle, hv_Row3, hv_Column3, hv_Angle);
	//获取变换后的测量模型的轮廓
	GetMetrologyObjectMeasures(&ho_LineContours, hv_MetrologyHandle, "all", "all",
		&hv_RowLine, &hv_ColumnLine);
	//卡尺找直线
	ApplyMetrologyModel(ho_Image, hv_MetrologyHandle);

	if (HDevWindowStack::IsOpen())
		SetColor(HDevWindowStack::GetActive(), "yellow");
	if (HDevWindowStack::IsOpen())
		SetLineWidth(HDevWindowStack::GetActive(), 2);
	if (HDevWindowStack::IsOpen())
		DispObj(ho_LineContours, HDevWindowStack::GetActive());
	GetMetrologyObjectResultContour(&ho_LineContour, hv_MetrologyHandle, 0, "all",
		1.5);
	//get_metrology_object_model_contour (ModelContour, MetrologyHandle, 'all', 1.5)
	if (HDevWindowStack::IsOpen())
		SetColor(HDevWindowStack::GetActive(), "blue");
	if (HDevWindowStack::IsOpen())
		SetLineWidth(HDevWindowStack::GetActive(), 2);
	if (HDevWindowStack::IsOpen())
		DispObj(ho_LineContour, HDevWindowStack::GetActive());
	FitLineContourXld(ho_LineContour, "tukey", -1, 0, 5, 2, &hv_RowBegin, &hv_ColBegin,
		&hv_RowEnd, &hv_ColEnd, &hv_Nr, &hv_Nc, &hv_Dist);
	//输出模板匹配后检测直线两端点的位置，即特征点位置。
	//double* TLineFP_result = new double[3];
	cout << " hv_result():" << hv_RowBegin[0].D() << "  " << hv_ColBegin[0].D() << "  " << hv_RowEnd[0].D() << "  " << hv_ColEnd[0].D() << endl;
	TLineFP_result[0] = hv_RowBegin[0].D();
	TLineFP_result[1] = hv_ColBegin[0].D();
	TLineFP_result[2] = hv_RowEnd[0].D();
	TLineFP_result[3] = hv_ColEnd[0].D();
	cout << hv_ModelFile << endl;
};

//模板匹配求倒角
double program_0::TemplateMatching_Chamfer_P(HObject ho_Image, HTuple hv_ModelFile, double ShapeModel1, double ShapeModel2, double ShapeModel3, double ShapeModel4, double ShapeModel5,
	int Rectangle1, int Rectangle2, int Rectangle3, int Rectangle4,
	int EdgesSubP1, int EdgesSubP2, int EdgesSubP3,
	bool Segment, int SegmentContour1, int SegmentContour2, int SegmentContour3,
	double SelectContour1, double SelectContour2, double SelectContour3,
	int UnionContour1, int UnionContour2,
	int FitCircleContour1, int FitCircleContour2, int FitCircleContour3, int FitCircleContour4, int FitCircleContour5)
{

	// Local iconic variables
	HObject  ho_ModelContours, ho_ContoursAffinTrans;
	HObject  ho_Rectangle, ho_RectangleTrans, ho_ImageReduced;
	HObject  ho_Edges, ho_ContoursSplit, ho_SelectedContours;
	HObject  ho_UnionContours;

	// Local control variables
	HTuple  hv_Width, hv_Height, hv_ReusedModelID;
	HTuple  hv_ReusedRefPointRow, hv_ReusedRefPointCol, hv_NumLevels;
	HTuple  hv_AngleStart, hv_AngleExtent, hv_AngleStep, hv_ScaleMin;
	HTuple  hv_ScaleMax, hv_ScaleStep, hv_Metric, hv_MinContrast;
	HTuple  hv_Row3, hv_Column3, hv_Angle, hv_Score, hv_HomMat2D;
	HTuple  hv_HomMat2DIdentity, hv_HomMat2DTranslate, hv_Row;
	HTuple  hv_Column, hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;

	//读取模板和图像进行模板匹配*
	GetImageSize(ho_Image, &hv_Width, &hv_Height);
	//hv_ModelFile = "./programParmeter/program0_1080new/cam0_4-220-72.sbm";
	ReadShapeModel(hv_ModelFile, &hv_ReusedModelID);
	//获取读取的模板的轮廓，区域坐标等信息
	GetShapeModelContours(&ho_ModelContours, hv_ReusedModelID, 1);
	GetShapeModelOrigin(hv_ReusedModelID, &hv_ReusedRefPointRow, &hv_ReusedRefPointCol);
	GetShapeModelParams(hv_ReusedModelID, &hv_NumLevels, &hv_AngleStart, &hv_AngleExtent,
		&hv_AngleStep, &hv_ScaleMin, &hv_ScaleMax, &hv_ScaleStep, &hv_Metric, &hv_MinContrast);
	//进行模板匹配
	//FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, 0.4,
	//	1, 0.3, "least_squares", 0, 0.6, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
	FindShapeModel(ho_Image, hv_ReusedModelID, hv_AngleStart, hv_AngleExtent, ShapeModel1,
		ShapeModel2, ShapeModel3, "least_squares", ShapeModel4, ShapeModel5, &hv_Row3, &hv_Column3, &hv_Angle, &hv_Score);
	//显示形状特征模板匹配结果
	//  
		//  
	VectorAngleToRigid(hv_ReusedRefPointRow, hv_ReusedRefPointCol, 0, hv_Row3, hv_Column3,
		hv_Angle, &hv_HomMat2D);
	AffineTransContourXld(ho_ModelContours, &ho_ContoursAffinTrans, hv_HomMat2D);
	if (HDevWindowStack::IsOpen())
		SetColor(HDevWindowStack::GetActive(), "green");
	if (HDevWindowStack::IsOpen())
		DispObj(ho_ContoursAffinTrans, HDevWindowStack::GetActive());
	//
	//Segment a region containing the edges
	//基于全局阈值的图像快速阈值化
	//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin与colend可固定不变


	//GenRectangle1(&ho_Rectangle, 0, 0, 300, 300);
	GenRectangle1(&ho_Rectangle, Rectangle1, Rectangle2, Rectangle3, Rectangle4);
	//设置条状的宽度为70

	HomMat2dIdentity(&hv_HomMat2DIdentity);
	HomMat2dTranslate(hv_HomMat2DIdentity, hv_Column3 - 1000, hv_Row3 + 1000, &hv_HomMat2DTranslate);

	AffineTransRegion(ho_Rectangle, &ho_RectangleTrans, hv_HomMat2DTranslate, "nearest_neighbor");

	ReduceDomain(ho_Image, ho_RectangleTrans, &ho_ImageReduced);
	//In the subdomain of the image containing the edges,
	//extract subpixel precise edges.
	//提取亚像素精密边缘轮廓
	//EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);
	EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", EdgesSubP1, EdgesSubP1, EdgesSubP1);
	if (Segment) {
		//SegmentContoursXld(ho_Edges, &ho_ContoursSplit, "lines_circles", 5, 4, 2);
		SegmentContoursXld(ho_Edges, &ho_ContoursSplit, "lines_circles", SegmentContour1, SegmentContour2, SegmentContour3);
		//提取出轮廓中较长的部分线段
		//SelectContoursXld(ho_ContoursSplit, &ho_SelectedContours, "contour_length", 20,
		//	hv_Width / 2, -0.5, 0.5);
		SelectContoursXld(ho_ContoursSplit, &ho_SelectedContours, "contour_length", SelectContour1,
			hv_Width / 2, SelectContour2, SelectContour3);
		//对相邻的轮廓段进行连接
		//UnionAdjacentContoursXld(ho_SelectedContours, &ho_UnionContours, 90, 1, "attr_keep");
		UnionAdjacentContoursXld(ho_SelectedContours, &ho_UnionContours, UnionContour1, UnionContour2, "attr_keep");
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		// 
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		//FitCircleContourXld(ho_UnionContours, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
		//	&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		FitCircleContourXld(ho_UnionContours, "algebraic", FitCircleContour1, FitCircleContour2, FitCircleContour3, FitCircleContour4, FitCircleContour5, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
	}
	else {
		//FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
		//	&hv_Radius, & hv_StartPhi, & hv_EndPhi, & hv_PointOrder);
		FitCircleContourXld(ho_Edges, "algebraic", FitCircleContour1, FitCircleContour2, FitCircleContour3, FitCircleContour4, FitCircleContour5, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
	}

	double Chamfer_Radius = hv_Radius.D();
	cout << "Chamfer_Radius():" << Chamfer_Radius << " " << Chamfer_Radius * 0.01216 << endl;
	cout << hv_ModelFile << endl;
	//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
	//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
	if (HDevWindowStack::IsOpen())
		SetColor(HDevWindowStack::GetActive(), "green");
	if (HDevWindowStack::IsOpen())
		DispObj(ho_Edges, HDevWindowStack::GetActive());
	return Chamfer_Radius;
};

//根据起始点与终止点求倒角
double program_0::TLineFP_Chamfer_P(HObject ho_Image, double GenRect1, double GenRect2, double GenRect3, double GenRect4,
	int EdgesSubP1, int EdgesSubP2, int EdgesSubP3,
	bool Segment, int SegmentContour1, int SegmentContour2, int SegmentContour3,
	double SelectContour1, double SelectContour2, double SelectContour3,
	int UnionContour1, int UnionContour2,
	int FitCircleContour1, int FitCircleContour2, int FitCircleContour3, int FitCircleContour4, int FitCircleContour5)
{

	// Local iconic variables
	HObject  ho_Rectangle, ho_ImageReduced;
	HObject  ho_Edges, ho_ContoursSplit, ho_SelectedContours;
	HObject  ho_UnionContours;

	// Local control variables
	HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
	HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;
	GetImageSize(ho_Image, &hv_Width, &hv_Height);

	//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
	//
	//Segment a region containing the edges
	//基于全局阈值的图像快速阈值化
	//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin与colend可固定不变
	//GenRectangle1(&ho_Rectangle, cam0_4_220_72shangbian_TLineFP[2], 500, cam0_4_220_72youbiann_TLineFP[0] - 20, 3500);
	GenRectangle1(&ho_Rectangle, GenRect1, GenRect2, GenRect3, GenRect4);
	//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
	ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);

	//In the subdomain of the image containing the edges,
	//extract subpixel precise edges.
	//提取亚像素精密边缘轮廓
	//EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", EdgesSubP1, EdgesSubP1, EdgesSubP1);
	EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);

	if (Segment) {
		SegmentContoursXld(ho_Edges, &ho_ContoursSplit, "lines_circles", 5, 4, 2);
		//SegmentContoursXld(ho_Edges, &ho_ContoursSplit, "lines_circles", SegmentContour1, SegmentContour2, SegmentContour3);
		//提取出轮廓中较长的部分线段
		SelectContoursXld(ho_ContoursSplit, &ho_SelectedContours, "contour_length", 20,
			hv_Width / 2, -0.5, 0.5);
		//SelectContoursXld(ho_ContoursSplit, &ho_SelectedContours, "contour_length", SelectContour1,
		//	hv_Width / 2, SelectContour2, SelectContour3);
		//对相邻的轮廓段进行连接
		UnionAdjacentContoursXld(ho_SelectedContours, &ho_UnionContours, 90, 1, "attr_keep");
		//UnionAdjacentContoursXld(ho_SelectedContours, &ho_UnionContours, UnionContour1, UnionContour2, "attr_keep");
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		// 
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_UnionContours, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		//FitCircleContourXld(ho_UnionContours, "algebraic", FitCircleContour1, FitCircleContour2, FitCircleContour3, FitCircleContour4, FitCircleContour5, &hv_Row, &hv_Column,
		//	&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cout << "Segment is true!" << endl;
	}
	else {
		//FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
		//	&hv_Radius, & hv_StartPhi, & hv_EndPhi, & hv_PointOrder);
		FitCircleContourXld(ho_Edges, "algebraic", FitCircleContour1, FitCircleContour2, FitCircleContour3, FitCircleContour4, FitCircleContour5, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
	}

	double Chamfer_Radius = hv_Radius.D();
	cout << "Chamfer_Radius():" << Chamfer_Radius << " " << Chamfer_Radius * 0.01216 << endl;
	//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
	//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
	if (HDevWindowStack::IsOpen())
		SetColor(HDevWindowStack::GetActive(), "green");
	if (HDevWindowStack::IsOpen())
		DispObj(ho_Edges, HDevWindowStack::GetActive());
	return Chamfer_Radius;
};


//根据起始点与终止点求倒角2 添加TupleMax(hv_Radius, &hv_Max); 选取最大值函数
double program_0::TLineFP_Chamfer_H2(HObject ho_Image, double GenRect1, double GenRect2, double GenRect3, double GenRect4,
	int EdgesSubP1, int EdgesSubP2, int EdgesSubP3,
	bool Segment, int SegmentContour1, int SegmentContour2, int SegmentContour3,
	double SelectContour1, double SelectContour2, double SelectContour3,
	int UnionContour1, int UnionContour2,
	int FitCircleContour1, int FitCircleContour2, int FitCircleContour3, int FitCircleContour4, int FitCircleContour5)
{

	// Local iconic variables
	HObject  ho_Rectangle, ho_ImageReduced;
	HObject  ho_Edges, ho_ContoursSplit, ho_SelectedContours;
	HObject  ho_UnionContours;
	HTuple  hv_Max;
	// Local control variables
	HTuple  hv_Width, hv_Height, hv_Row, hv_Column;
	HTuple  hv_Radius, hv_StartPhi, hv_EndPhi, hv_PointOrder;
	HObject ho_GammaImage, ho_ImageEmphasize, ho_ImageGauss;
	GetImageSize(ho_Image, &hv_Width, &hv_Height);
	GammaImage(ho_Image, &ho_GammaImage, 0.616667, 0.055, 0.018, 255, "true");
	Emphasize(ho_GammaImage, &ho_ImageEmphasize, hv_Width, hv_Height, 1.4);
	GaussFilter(ho_ImageEmphasize, &ho_ImageGauss, 7);
	//dev_open_window (0, 0, Width, Height, 'black', WindowHandle)
	//
	//Segment a region containing the edges
	//基于全局阈值的图像快速阈值化
	//Rowbegin和Rowend是倒角两端特征点的row位置，colbegin与colend可固定不变
	//GenRectangle1(&ho_Rectangle, cam0_4_220_72shangbian_TLineFP[2], 500, cam0_4_220_72youbiann_TLineFP[0] - 20, 3500);
	GenRectangle1(&ho_Rectangle, GenRect1, GenRect2, GenRect3, GenRect4);
	//提取roi区域，roi以外的变黑，不改变整个图像的尺寸大小。corp_domain是只提取roi区域
	ReduceDomain(ho_ImageGauss, ho_Rectangle, &ho_ImageReduced);

	//In the subdomain of the image containing the edges,
	//extract subpixel precise edges.
	//提取亚像素精密边缘轮廓
	//EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", EdgesSubP1, EdgesSubP1, EdgesSubP1);
	EdgesSubPix(ho_ImageReduced, &ho_Edges, "canny", 2, 20, 60);

	if (Segment) {
		SegmentContoursXld(ho_Edges, &ho_ContoursSplit, "lines_circles", 5, 4, 2);
		//SegmentContoursXld(ho_Edges, &ho_ContoursSplit, "lines_circles", SegmentContour1, SegmentContour2, SegmentContour3);
		//提取出轮廓中较长的部分线段
		SelectContoursXld(ho_ContoursSplit, &ho_SelectedContours, "contour_length", 20,
			hv_Width / 2, -0.5, 0.5);
		//SelectContoursXld(ho_ContoursSplit, &ho_SelectedContours, "contour_length", SelectContour1,
		//	hv_Width / 2, SelectContour2, SelectContour3);
		//对相邻的轮廓段进行连接
		UnionAdjacentContoursXld(ho_SelectedContours, &ho_UnionContours, 90, 1, "attr_keep");
		//UnionAdjacentContoursXld(ho_SelectedContours, &ho_UnionContours, UnionContour1, UnionContour2, "attr_keep");
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		// 
		//将两特征点中间圆弧处进行曲线拟合 输出Radius*0.01216就是真实尺寸
		FitCircleContourXld(ho_UnionContours, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		//FitCircleContourXld(ho_UnionContours, "algebraic", FitCircleContour1, FitCircleContour2, FitCircleContour3, FitCircleContour4, FitCircleContour5, &hv_Row, &hv_Column,
		//	&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
		cout << "Segment is true!" << endl;
	}
	else {
		//FitCircleContourXld(ho_Edges, "algebraic", -1, 0, 0, 3, 2, &hv_Row, &hv_Column,
		//	&hv_Radius, & hv_StartPhi, & hv_EndPhi, & hv_PointOrder);
		FitCircleContourXld(ho_Edges, "algebraic", FitCircleContour1, FitCircleContour2, FitCircleContour3, FitCircleContour4, FitCircleContour5, &hv_Row, &hv_Column,
			&hv_Radius, &hv_StartPhi, &hv_EndPhi, &hv_PointOrder);
	}
	TupleMax(hv_Radius, &hv_Max);
	double Chamfer_Radius = hv_Max.D() - 26;
	cout << "Chamfer_Radius():" << Chamfer_Radius << " " << Chamfer_Radius * 0.00693 << endl;
	//将一个XLD轮廓分割为直线段、圆（圆弧)、椭圆弧
	//segment_contours_xld (Edges, ContoursSplit, 'lines_circles', 5, 4, 3)
	if (HDevWindowStack::IsOpen())
		SetColor(HDevWindowStack::GetActive(), "green");
	if (HDevWindowStack::IsOpen())
		DispObj(ho_Edges, HDevWindowStack::GetActive());
	return Chamfer_Radius;
};


HObject program_0::imgAug(string imgPath) {
	HObject  ho_Image, ho_LineContours, ho_LineContour;
	// Local control variables
	HTuple  hv_Width, hv_Height;
	HTuple  halconPath = imgPath.c_str();
	ReadImage(&ho_Image, halconPath);
	GetImageSize(ho_Image, &hv_Width, &hv_Height);
	//图像增强
	HObject   ho_GammaImage, ho_ImageEmphasize;
	GammaImage(ho_Image, &ho_GammaImage, 0.416667, 0.055, 0.018, 255, "true");
	Emphasize(ho_GammaImage, &ho_ImageEmphasize, hv_Width, hv_Height, 1.4);
	//图像增强结束
	return ho_ImageEmphasize;
}




void program_0::on_test_clicked()
{
	

	
	//newProgram0_4.cpp begin 155轴
	
	string Img1path = "C:/Users/11267/Desktop/halcon331/features/biaoding/1.bmp";
	string Img2path = "C:/Users/11267/Desktop/halcon331/features/biaoding/2.bmp";
	string Img3path = "C:/Users/11267/Desktop/halcon331/features/biaoding/3.bmp";
	string Img4path = "C:/Users/11267/Desktop/halcon331/features/biaoding/4.bmp";
	string Img5path = "C:/Users/11267/Desktop/halcon331/features/biaoding/5.bmp";

	HObject Img1Aug = imgAug(Img1path);
	HObject Img2Aug = imgAug(Img2path);
	HObject Img3Aug = imgAug(Img3path);
	HObject Img4Aug = imgAug(Img4path);
	HObject Img5Aug = imgAug(Img5path);

	HTuple halconPath = Img1path.c_str();
	HObject Img1_raw;
	ReadImage(&Img1_raw, halconPath);

	halconPath = Img2path.c_str();
	HObject Img2_raw;
	ReadImage(&Img2_raw, halconPath);

	halconPath = Img3path.c_str();
	HObject Img3_raw;
	ReadImage(&Img3_raw, halconPath);

	halconPath = Img4path.c_str();
	HObject Img4_raw;
	ReadImage(&Img4_raw, halconPath);

	halconPath = Img5path.c_str();
	HObject Img5_raw;
	ReadImage(&Img5_raw, halconPath);
	

	int fault_detect = 0;

	double cam0_1_0_TLineFP[4];
	Straight_TLineFP_P(cam0_1_0_TLineFP, Img1Aug,
		244, 900, 244, 1000,
		50, 12, 1, 1);
	
	double cam0_1_15_TLineFP[4];
	Straight_TLineFP_P(cam0_1_15_TLineFP, Img1Aug,
		1481, 900, 1481, 1000,
		50, 12, 1, 1);

	double cam0_1_25_TLineFP[4];
	Straight_TLineFP_P(cam0_1_25_TLineFP, Img1Aug,
		2295, 900, 2295, 1000,
		50, 12, 1, 1);
	
	double cam0_1_35_TLineFP[4];
	Straight_TLineFP_P(cam0_1_35_TLineFP, Img1Aug,
		3122, 900, 3122, 1000,
		50, 12, 1, 1);
	
	double cam0_1_45_TLineFP[4];
	Straight_TLineFP_P(cam0_1_45_TLineFP, Img1Aug,
		3936, 900, 3936, 1000,
		50, 12, 1, 1);

	double cam0_1_55_TLineFP[4];
	Straight_TLineFP_P(cam0_1_55_TLineFP, Img1Aug,
		4763, 900, 4763, 1000,
		50, 12, 1, 1);

	//图像2
	double cam0_2_65_TLineFP[4];
	Straight_TLineFP_P(cam0_2_65_TLineFP, Img2Aug,
		573, 900, 573, 1000,
		50, 12, 1, 1);

	double cam0_2_75_TLineFP[4];
	Straight_TLineFP_P(cam0_2_75_TLineFP, Img2Aug,
		1393, 1000, 1393, 1070,
		50, 12, 1, 1);

	double cam0_2_85_TLineFP[4];
	Straight_TLineFP_P(cam0_2_85_TLineFP, Img2Aug,
		2213, 800, 2213, 900,
		50, 12, 1, 1);

	double cam0_2_95_TLineFP[4];
	Straight_TLineFP_P(cam0_2_95_TLineFP, Img2Aug,
		3033, 900, 3033, 1000,
		50, 12, 1, 1);

	double cam0_2_105_TLineFP[4];
	Straight_TLineFP_P(cam0_2_105_TLineFP, Img2Aug,
		3853, 900, 3853, 1000,
		50, 12, 1, 1);

	double cam0_2_115_TLineFP[4];
	Straight_TLineFP_P(cam0_2_115_TLineFP, Img2Aug,
		4673, 900, 4673, 1000,
		50, 12, 1, 1);
	//图像3
	double cam0_3_125_TLineFP[4];
	Straight_TLineFP_P(cam0_3_125_TLineFP, Img3Aug,
		573, 990, 573, 1070,
		50, 12, 1, 1);

	double cam0_3_135_TLineFP[4];
	Straight_TLineFP_P(cam0_3_135_TLineFP, Img3Aug,
		1393, 900, 1393, 1000,
		50, 12, 1, 1);

	double cam0_3_145_TLineFP[4];
	Straight_TLineFP_P(cam0_3_145_TLineFP, Img3Aug,
		2213, 990, 2213, 1070,
		50, 12, 1, 1);

	double cam0_3_155_TLineFP[4];
	Straight_TLineFP_P(cam0_3_155_TLineFP, Img3Aug,
		3033, 900, 3033, 1000,
		50, 12, 1, 1);

	double cam0_3_165_TLineFP[4];
	Straight_TLineFP_P(cam0_3_165_TLineFP, Img3Aug,
		3853, 990, 3853, 1070,
		50, 12, 1, 1);

	double cam0_3_175_TLineFP[4];
	Straight_TLineFP_P(cam0_3_175_TLineFP, Img3Aug,
		4673, 900, 4673, 1000,
		50, 12, 1, 1);

	//图像4
	double cam0_4_185_TLineFP[4];
	Straight_TLineFP_P(cam0_4_185_TLineFP, Img4Aug,
		573, 990, 573, 1070,
		50, 12, 1, 1);

	double cam0_4_195_TLineFP[4];
	Straight_TLineFP_P(cam0_4_195_TLineFP, Img4Aug,
		1393, 900, 1393, 1000,
		50, 12, 1, 1);

	double cam0_4_205_TLineFP[4];
	Straight_TLineFP_P(cam0_4_205_TLineFP, Img4Aug,
		2213, 990, 2213, 1070,
		50, 12, 1, 1);

	double cam0_4_215_TLineFP[4];
	Straight_TLineFP_P(cam0_4_215_TLineFP, Img4Aug,
		3033, 900, 3033, 1000,
		50, 12, 1, 1);

	double cam0_4_225_TLineFP[4];
	Straight_TLineFP_P(cam0_4_225_TLineFP, Img4Aug,
		3853, 990, 3853, 1070,
		50, 12, 1, 1);

	double cam0_4_235_TLineFP[4];
	Straight_TLineFP_P(cam0_4_235_TLineFP, Img4Aug,
		4673, 900, 4673, 1000,
		50, 12, 1, 1);

	//图像5
	double cam0_5_245_TLineFP[4];
	Straight_TLineFP_P(cam0_5_245_TLineFP, Img5Aug,
		410, 990, 410, 1070,
		50, 12, 1, 1);

	double cam0_5_255_TLineFP[4];
	Straight_TLineFP_P(cam0_5_255_TLineFP, Img5Aug,
		1233, 900, 1233, 1000,
		50, 12, 1, 1);

	double cam0_5_265_TLineFP[4];
	Straight_TLineFP_P(cam0_5_265_TLineFP, Img5Aug,
		2053, 990, 2053, 1070,
		50, 12, 1, 1);

	double cam0_5_275_TLineFP[4];
	Straight_TLineFP_P(cam0_5_275_TLineFP, Img5Aug,
		2873, 900, 2873, 1000,
		50, 12, 1, 1);

	double cam0_5_285_TLineFP[4];
	Straight_TLineFP_P(cam0_5_285_TLineFP, Img5Aug,
		3693, 990, 3693, 1070,
		50, 12, 1, 1);

	double cam0_5_300_TLineFP[4];
	Straight_TLineFP_P(cam0_5_300_TLineFP, Img5Aug,
		4927, 980, 4927, 1050,
		50, 12, 1, 1);


	//以下程序对特征集中处理，不涉及特征点检测
	
	double AxialD_biaozhun_15 = (cam0_1_15_TLineFP[0]+ cam0_1_15_TLineFP[2])/2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_15* Calik);
	pushback_vectors(resultVectorList, 4, 15, 15, -1, +1);

	double AxialD_biaozhun_25 = (cam0_1_25_TLineFP[0] + cam0_1_25_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_25* Calik);
	pushback_vectors(resultVectorList, 4, 25, 25, -1, +1);

	double AxialD_biaozhun_35 = (cam0_1_35_TLineFP[0] + cam0_1_35_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_35* Calik);
	pushback_vectors(resultVectorList, 4, 35, 35, -1, +1);

	double AxialD_biaozhun_45 = (cam0_1_45_TLineFP[0] + cam0_1_45_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_45* Calik);
	pushback_vectors(resultVectorList, 4, 45, 45, -1, +1);

	double AxialD_biaozhun_55 = (cam0_1_55_TLineFP[0] + cam0_1_55_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_55* Calik);
	pushback_vectors(resultVectorList, 4, 55, 55, -1, +1);

	//图片2
	double AxialD_biaozhun_65 = (cam0_2_65_TLineFP[0] + cam0_2_65_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_65* Calik+pic1To2_moveDistance);
	pushback_vectors(resultVectorList, 4, 65, 65, -1, +1);

	double AxialD_biaozhun_75 = (cam0_2_75_TLineFP[0] + cam0_2_75_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_75* Calik + pic1To2_moveDistance);
	pushback_vectors(resultVectorList, 4, 75, 75, -1, +1);

	double AxialD_biaozhun_85 = (cam0_2_85_TLineFP[0] + cam0_2_85_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_85* Calik + pic1To2_moveDistance);
	pushback_vectors(resultVectorList, 4, 85, 85, -1, +1);

	double AxialD_biaozhun_95 = (cam0_2_95_TLineFP[0] + cam0_2_95_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_95* Calik + pic1To2_moveDistance);
	pushback_vectors(resultVectorList, 4, 95, 95, -1, +1);

	double AxialD_biaozhun_105 = (cam0_2_105_TLineFP[0] + cam0_2_105_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_105* Calik + pic1To2_moveDistance);
	pushback_vectors(resultVectorList, 4, 105, 105, -1, +1);

	double AxialD_biaozhun_115 = (cam0_2_115_TLineFP[0] + cam0_2_115_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_115* Calik + pic1To2_moveDistance);
	pushback_vectors(resultVectorList, 4, 115, 115, -1, +1);
	
	//图片3
	double AxialD_biaozhun_125 = (cam0_3_125_TLineFP[0] + cam0_3_125_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_125* Calik + pic1To2_moveDistance+ pic2To3_moveDistance);
	pushback_vectors(resultVectorList, 4, 125, 125, -1, +1);

	double AxialD_biaozhun_135 = (cam0_3_135_TLineFP[0] + cam0_3_135_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_135* Calik + pic1To2_moveDistance + pic2To3_moveDistance);
	pushback_vectors(resultVectorList, 4, 135, 135, -1, +1);

	double AxialD_biaozhun_145 = (cam0_3_145_TLineFP[0] + cam0_3_145_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_145* Calik + pic1To2_moveDistance + pic2To3_moveDistance);
	pushback_vectors(resultVectorList, 4, 145, 145, -1, +1);

	double AxialD_biaozhun_155 = (cam0_3_155_TLineFP[0] + cam0_3_155_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_155* Calik + pic1To2_moveDistance + pic2To3_moveDistance);
	pushback_vectors(resultVectorList, 4, 155, 155, -1, +1);

	double AxialD_biaozhun_165 = (cam0_3_165_TLineFP[0] + cam0_3_165_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_165* Calik + pic1To2_moveDistance + pic2To3_moveDistance);
	pushback_vectors(resultVectorList, 4, 165, 165, -1, +1);

	double AxialD_biaozhun_175 = (cam0_3_175_TLineFP[0] + cam0_3_175_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_175* Calik + pic1To2_moveDistance + pic2To3_moveDistance);
	pushback_vectors(resultVectorList, 4, 175, 175, -1, +1);

	//图片4
	double AxialD_biaozhun_185 = (cam0_4_185_TLineFP[0] + cam0_4_185_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_185* Calik + pic1To2_moveDistance + pic2To3_moveDistance+ pic3To4_moveDistance);
	pushback_vectors(resultVectorList, 4, 185, 185, -1, +1);

	double AxialD_biaozhun_195 = (cam0_4_195_TLineFP[0] + cam0_4_195_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_195* Calik + pic1To2_moveDistance + pic2To3_moveDistance + pic3To4_moveDistance);
	pushback_vectors(resultVectorList, 4, 195, 195, -1, +1);

	double AxialD_biaozhun_205 = (cam0_4_205_TLineFP[0] + cam0_4_205_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_205* Calik + pic1To2_moveDistance + pic2To3_moveDistance + pic3To4_moveDistance);
	pushback_vectors(resultVectorList, 4, 205, 205, -1, +1);

	double AxialD_biaozhun_215 = (cam0_4_215_TLineFP[0] + cam0_4_215_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_215* Calik + pic1To2_moveDistance + pic2To3_moveDistance + pic3To4_moveDistance);
	pushback_vectors(resultVectorList, 4, 215, 215, -1, +1);

	double AxialD_biaozhun_225 = (cam0_4_225_TLineFP[0] + cam0_4_225_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_225* Calik + pic1To2_moveDistance + pic2To3_moveDistance + pic3To4_moveDistance);
	pushback_vectors(resultVectorList, 4, 225, 225, -1, +1);

	double AxialD_biaozhun_235 = (cam0_4_235_TLineFP[0] + cam0_4_235_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_235* Calik + pic1To2_moveDistance + pic2To3_moveDistance + pic3To4_moveDistance);
	pushback_vectors(resultVectorList, 4, 235, 235, -1, +1);

	//图片5
	double AxialD_biaozhun_245 = (cam0_5_245_TLineFP[0] + cam0_5_245_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_245* Calik + pic1To2_moveDistance + pic2To3_moveDistance + pic3To4_moveDistance + pic4To5_moveDistance);
	pushback_vectors(resultVectorList, 4, 245, 245, -1, +1);

	double AxialD_biaozhun_255 = (cam0_5_255_TLineFP[0] + cam0_5_255_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_255* Calik + pic1To2_moveDistance + pic2To3_moveDistance + pic3To4_moveDistance + pic4To5_moveDistance);
	pushback_vectors(resultVectorList, 4, 255, 255, -1, +1);

	double AxialD_biaozhun_265 = (cam0_5_265_TLineFP[0] + cam0_5_265_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_265* Calik + pic1To2_moveDistance + pic2To3_moveDistance + pic3To4_moveDistance + pic4To5_moveDistance);
	pushback_vectors(resultVectorList, 4, 265, 265, -1, +1);

	double AxialD_biaozhun_275 = (cam0_5_275_TLineFP[0] + cam0_5_275_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_275* Calik + pic1To2_moveDistance + pic2To3_moveDistance + pic3To4_moveDistance + pic4To5_moveDistance);
	pushback_vectors(resultVectorList, 4, 275, 275, -1, +1);

	double AxialD_biaozhun_285 = (cam0_5_285_TLineFP[0] + cam0_5_285_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_285* Calik + pic1To2_moveDistance + pic2To3_moveDistance + pic3To4_moveDistance + pic4To5_moveDistance);
	pushback_vectors(resultVectorList, 4, 285, 285, -1, +1);

	double AxialD_biaozhun_300 = (cam0_5_300_TLineFP[0] + cam0_5_300_TLineFP[2]) / 2 - (cam0_1_0_TLineFP[0] + cam0_1_0_TLineFP[2]) / 2;
	resultVectorList.push_back(AxialD_biaozhun_300* Calik + pic1To2_moveDistance + pic2To3_moveDistance + pic3To4_moveDistance + pic4To5_moveDistance);
	pushback_vectors(resultVectorList, 4, 300, 300, -1, +1);

}; 


void program_0::saveAsExcel()//将主界面上的表格保存为excel文件
{
	string excelPath = "E:/331after/331MiddleCheck/measureResult3.xlsx";
	QAxObject* excel = new QAxObject;
	if (excel->setControl("Excel.Application"))
	{
		excel->dynamicCall("SetVisible (bool Visible)", false);
		excel->setProperty("DisplayAlerts", false);
		QAxObject* workbooks = excel->querySubObject("WorkBooks");            //获取工作簿集合
		workbooks->dynamicCall("Add");                                        //新建一个工作簿
		QAxObject* workbook = excel->querySubObject("ActiveWorkBook");        //获取当前工作簿
		QAxObject* worksheet = workbook->querySubObject("Worksheets(int)", 1);
		QAxObject* cell;
		int rowCount = measureTablePtr->rowCount();
		int columnCount = measureTablePtr->columnCount();
		//添加Excel表头数据
		for (int i = 1; i <= columnCount; i++)
		{
			cell = worksheet->querySubObject("Cells(int,int)", 1, i);
			cell->setProperty("RowHeight", 40);
			QString st1 = (measureTablePtr->horizontalHeaderItem(i - 1)->data(0)).toString();

			cell->dynamicCall("SetValue(const QString&)", measureTablePtr->horizontalHeaderItem(i - 1)->data(0).toString());
			cell->querySubObject("Font")->setProperty("Bold", true);
			cell->querySubObject("Interior")->setProperty("Color", QColor(220, 220, 220));

		};
		//将数据写入excel
		for (int j = 2; j <= rowCount + 1; j++)
		{

			cell = worksheet->querySubObject("Cells(int,int)", j, 1);
			cell->dynamicCall("SetValue(const float)", measureTablePtr->item(j - 2, 0)->text() + "\t");
			cell = worksheet->querySubObject("Cells(int,int)", j, 2);
			cell->dynamicCall("SetValue(const QString&)", measureTablePtr->item(j - 2, 1)->text() + "\t");
			cell = worksheet->querySubObject("Cells(int,int)", j, 3);
			cell->dynamicCall("SetValue(const QString&)", measureTablePtr->item(j - 2, 2)->text() + "\t");
			for (int k = 4; k <= measureTablePtr->columnCount(); k++)
			{
				cell = worksheet->querySubObject("Cells(int,int)", j, k);
				cell->dynamicCall("SetValue(const float)", measureTablePtr->item(j - 2, k - 1)->text() + "\t");
			};

		};
		QString fileName = QString(QString::fromLocal8Bit(excelPath.c_str()));
		workbook->dynamicCall("SaveAs(const QString&)", QDir::toNativeSeparators(fileName)); //保存至fileName
		workbook->dynamicCall("Close()");                                                   //关闭工作簿
		excel->dynamicCall("Quit()");                                                       //关闭excel
		delete excel;
		excel = NULL;
	};
};
import sys
import os
import cv2
import numpy as np
import tensorflow as tf

from tensorflow.keras.models import load_model

def preprocess_image(image_path):
    img = cv2.imread(image_path, cv2.IMREAD_GRAYSCALE)
    img_rgb = cv2.cvtColor(img, cv2.COLOR_GRAY2RGB)
    img_rgb = cv2.resize(img_rgb, (512, 512))
    img_rgb = img_rgb.reshape((1, 512, 512, 3))
    img_rgb = img_rgb.astype('float32') / 255.0
    return img_rgb

# 从命令行参数获取图像路径和模型路径
image_path = sys.argv[1]
model_path = sys.argv[2]


# 加载模型
model = load_model(model_path, compile=False)

# 预处理图像
preprocessed_image = preprocess_image(image_path)

# 进行预测
prediction = model.predict(preprocessed_image)[0][0]
prediction *= 1.334

# 将预测结果的数字部分作为输出返回给Qt应用程序
print(prediction)

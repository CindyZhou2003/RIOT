import glob
import numpy as np
# 训练集、测试集划分
from sklearn.model_selection import train_test_split
# 数据集相对路径
DATA_PATH = "../../../10_tingml_datasets/"
# LABELS 的内容尽量与前面store_data.py保持一致
LABELS = ["Stationary_h","Stationary_v","Tilted", "Rotating", "Moving_h","Moving_v"]
# 代表一个样本内容，如连续10次传感器读到的6轴数据作为一个样本
SAMPLES_PER_GESTURE = 10
def load_one_label_data(label):
    path = DATA_PATH + label + "*.npy"
    files = glob.glob(path)
    datas = []
    for file in files:
        try:
            data = np.load(file)
            # 切除多余数据，如数据当中有61份，但每个样本只需要10份，那么最后一份需要丢弃。
            num_slice = len(data) // SAMPLES_PER_GESTURE
            datas.append(data[: num_slice * SAMPLES_PER_GESTURE, :])
        except Exception as e:
            print(e)
    datas = np.concatenate(datas, axis=0)
    # 由于本案例给的是全连接层，输入为1维数据。(其余如conv需要自行根据模型输入修改尺寸，如二维)
    # MLP
    # datas = np.reshape(datas,(-1, 6 * SAMPLES_PER_GESTURE,),)  # Modified here
    # CNN 1
    datas = np.reshape(datas,(-1, 6 * SAMPLES_PER_GESTURE, 1),)  # Modified here
    idx = LABELS.index(label)
    labels = np.ones(datas.shape[0]) * idx
    return datas, labels
all_datas = []
all_labels = []
# 导入每个label对应的数据
for label in LABELS:
    datas, labels = load_one_label_data(label)
    all_datas.append(datas)
    all_labels.append(labels)
dataX = np.concatenate(all_datas, axis=0)
dataY = np.concatenate(all_labels, axis=0)
# 输入和样本到此创建完毕

# 训练集、测试集划分
# test_size 表示数据集里面有20%将划分给测试集
# stratify=dataY指定按label进行划分, 确保数据集划分公平
xTrain, xTest, yTrain, yTest = train_test_split(
    dataX, dataY, test_size=0.2, stratify=dataY
)
print(xTrain.shape, xTest.shape, yTrain.shape, yTest.shape)


import os
# 0 = INFO, 1 = WARNING, 2 = ERROR, 3 = FATAL
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '2'
import tensorflow as tf
import tensorflow.keras as keras

# 设置环境变量，控制日志级别
def mlp():
    # 一个用于线性堆叠多个网络层的模型。
    # Sequential模型是最简单的神经网络模型，它按照层的顺序依次堆叠，每一层的输出会成为下一层的输入。
    model = keras.Sequential()
    # 第一层, 添加全连接层，输出尺寸为64，激活函数采用"relu"
    # 第一层需要制定输入大小，这里和数据集对应input_shape=(6 * SAMPLES_PER_GESTURE,)
    model.add(keras.layers.Dense(64, activation="relu", input_shape=(6 * SAMPLES_PER_GESTURE,)))
    # 添加池化层，防止模型过拟合，每次自动忘记20%的参数
    model.add(keras.layers.Dropout(0.2))
    # 最后一层，全连接层，输出尺寸对应labels数量，激活函数采用"softmax"
    # softmaxs输出的结果代表每个label的概率，如第0个代表label 0的概率
    model.add(keras.layers.Dense(len(LABELS), activation="softmax"))
    return model
def cnn():
    # 一个用于线性堆叠多个网络层的模型。
    # Sequential模型是最简单的神经网络模型，它按照层的顺序依次堆叠，每一层的输出会成为下一层的输入。
    model = keras.Sequential()
    # 注意CNN与MLP的输入shape
    # 16个输出通道，3为卷积核大小
    model.add(
        keras.layers.Conv1D(
            8,3,padding="same",activation="relu",input_shape=(6 * SAMPLES_PER_GESTURE, 1),
        )
    )
    model.add(keras.layers.Conv1D(8, 3, padding="same", activation="relu"))
    model.add(keras.layers.GlobalAveragePooling1D())
    model.add(keras.layers.Dense(8, activation="relu"))
    model.add(keras.layers.Dropout(0.2))
    model.add(keras.layers.Dense(len(LABELS), activation="softmax"))
    return model

'''
CNN可以有效提取局部特征，而LSTM可以捕捉时间维度上的长时依赖关系。
对于运动数据，这样的组合往往能够带来更好的性能。
'''
def cnn_lstm():
    model = keras.Sequential()
    
    # 卷积层，用于提取空间特征
    model.add(keras.layers.Conv1D(
        filters=16, kernel_size=3, padding="same", activation="relu",
        input_shape=(6 * SAMPLES_PER_GESTURE, 1)
    ))
    model.add(keras.layers.Conv1D(32, kernel_size=3, padding="same", activation="relu"))
    model.add(keras.layers.MaxPooling1D(pool_size=2))
    
    # LSTM层，用于捕捉时序信息
    model.add(keras.layers.LSTM(64, return_sequences=True))
    model.add(keras.layers.LSTM(64))
    
    # Dropout层，防止过拟合
    model.add(keras.layers.Dropout(0.5))
    
    # 全连接层
    model.add(keras.layers.Dense(64, activation="relu"))
    
    # 输出层，使用softmax进行分类
    model.add(keras.layers.Dense(len(LABELS), activation="softmax"))
    
    # 编译模型
    model.compile(
        optimizer='adam',
        loss='categorical_crossentropy',
        metrics=['accuracy']
    )
    
    return model

model = cnn()
# model = mlp()
# model = cnn_lstm()
# 打印模型结构
model.summary()


from tensorflow.keras.callbacks import ModelCheckpoint
# 加载模型
from tensorflow.keras.models import load_model
# 测试模型性能
from sklearn.metrics import confusion_matrix
# 模型训练优化器，学习率为0.001
optimizer = keras.optimizers.Adam(lr=0.001)
# 制定模型优化器，和损失函数、评价指标
model.compile(
    loss="sparse_categorical_crossentropy",
    optimizer=optimizer,
    metrics=["sparse_categorical_accuracy"],
)
# 制定保存模型的路径
filepath = "best_model.h5"
# 训练时，保存最好模型
checkpoint = ModelCheckpoint(
    filepath,
    monitor="val_sparse_categorical_accuracy",
    verbose=1,
    save_best_only=True,
    mode="max",
)
# 模型训练, 训练集，batch_size为批大小，可提高训练速度
# validation_data指明验证集，epochs表示训练迭代轮数
# verbose=1表示打印训练日志
# callbacks调用上述保存模型的方法
history = model.fit(
    xTrain,
    yTrain,
    batch_size=8,
    validation_data=(xTest, yTest),
    epochs=50,
    verbose=1,
    callbacks=[checkpoint],
)
# 至此模型训练完毕


# 加载模型
model = load_model(filepath)
# 模型推理，预测
predictions = model.predict(xTest)
predictions = np.argmax(predictions, axis=1)
# 查看混淆矩阵，效果越好，预测则集中在对角线。
cm = confusion_matrix(yTest, predictions)
print(cm)


# Convert the model to the TensorFlow Lite format with quantization
# 加载模型
model = load_model(filepath)
converter = tf.lite.TFLiteConverter.from_keras_model(model)

# Enable experimental resource variables handling
converter.experimental_enable_resource_variables = True

# Disable lowering of tensor list operations (to avoid the TensorListReserve issue)
converter._experimental_lower_tensor_list_ops = False

tflite_model = converter.convert()
# 保存初始版本，后续对比用
open("model_basic.tflite", "wb").write(tflite_model)

# 量化模型, 定义输入格式与大小，只需要修改(-1, 6 * SAMPLES_PER_GESTURE,)与上面对应即可，其余不用变
data_test = xTest.astype("float32")
# np.reshape 和一开始数据集导入对应
# MLP
# data_test = np.reshape(data_test, (-1, 6 * SAMPLES_PER_GESTURE, ))
# CNN 1
data_test = np.reshape(data_test, (-1, 6 * SAMPLES_PER_GESTURE, 1))
data_ds = tf.data.Dataset.from_tensor_slices((data_test)).batch(1)
# Rest of your code...
def representative_data_gen():
    for input_value in data_ds.take(100):
        yield [input_value]
converter.representative_dataset = representative_data_gen
converter.optimizations = [tf.lite.Optimize.OPTIMIZE_FOR_SIZE]
tflite_model = converter.convert()
open("model.tflite", "wb").write(tflite_model)

# 量化前后对比
basic_model_size = os.path.getsize("model_basic.tflite")
print("Basic model is %d bytes" % basic_model_size)
quantized_model_size = os.path.getsize("model.tflite")
print("Quantized model is %d bytes" % quantized_model_size)
difference = basic_model_size - quantized_model_size
print("Difference is %d bytes" % difference)




# Now let's verify the model on a few input digits
# Instantiate an interpreter for the model
model_quantized_reloaded = tf.lite.Interpreter("model.tflite")

# Allocate memory for each model
model_quantized_reloaded.allocate_tensors()

# Get the input and output tensors so we can feed in values and get the results
model_quantized_input = model_quantized_reloaded.get_input_details()[0]["index"]
model_quantized_output = model_quantized_reloaded.get_output_details()[0]["index"]
# Create arrays to store the results
model_quantized_predictions = np.empty(xTest.size)
for i in range(20):
    # Reshape the data and ensure the type is float32
    # test_data = np.reshape(
    #     xTest[i],
    #     (
    #         1,
    #         6 * SAMPLES_PER_GESTURE,
    #         1,
    #     ),
    # ).astype("float32")
    test_data = np.expand_dims(xTest[i], axis=0).astype("float32")
    print(test_data.shape)
    # Invoke the interpreter
    model_quantized_reloaded.set_tensor(model_quantized_input, test_data)
    model_quantized_reloaded.invoke()
    model_quantized_prediction = model_quantized_reloaded.get_tensor(
        model_quantized_output
    )

    print("Digit: {} - Prediction:\n{}".format(yTest[i], model_quantized_prediction))
    print("")

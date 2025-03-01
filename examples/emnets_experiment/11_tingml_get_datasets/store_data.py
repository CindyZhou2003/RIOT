# -*- coding: utf-8 -*-

import os  # 用于操作文件和目录
import sys  # 用于系统操作，如退出程序
import glob  # 用于查找符合特定模式的文件
import serial  # 用于串口通信
import argparse
import numpy as np  # 用于数值计算和数组处理
from datetime import datetime  # 用于获取当前日期和时间

# 最大记录数
MAX_NUM = 200


class SerialControl:
    def __init__(self, port="/dev/ttyUSB0", direction="none"):
        # 查找所有符合 /dev/ttyUSB* 模式的串口设备
        ports = glob.glob("/dev/ttyUSB*")
        if not ports:
            print("No Serial Port available")
            sys.exit(1)  # 没有可用串口时退出程序

        self.port = ports[0]  # 使用第一个找到的串口
        self.serial = serial.Serial(
            self.port, 115200, timeout=1
        )  # 打开串口，波特率为115200，超时设置为1秒
        if self.serial.isOpen():
            print(f"{self.port} is connected")  # 如果串口打开，打印连接信息

        self.history = []  # 初始化历史记录列表
        self.direction = direction  # 设定方向属性

    def get_log(self):
        history = []  # 用于存储读取的数据

        # 读取数据直到达到最大记录数
        while len(history) < MAX_NUM:
            try:
                response = (
                    self.serial.readline().decode().strip()
                )  # 从串口读取一行并解码
                if not response:
                    continue  # 如果读取到空行，跳过当前循环

                if response.startswith("[main]"):  # 检查是否以"[main]"开头
                    response_list = response.split(":")[
                        2:-1
                    ]  # 分割字符串并跳过开头和结尾
                    acceler_xyz = response_list[0:3]
                    gryo_xyz = response_list[4:7]
                    xyz = []
                    xyz.extend(acceler_xyz)
                    xyz.extend(gryo_xyz)
                    sub_history = [float(x) for x in xyz]

                    print(len(history), sub_history)  # 打印当前记录数和子记录
                    history.append(sub_history)  # 将子记录添加到历史记录列表中
            except (UnicodeDecodeError, ValueError) as e:
                print(
                    f"Error decoding or parsing response: {e}"
                )  # 捕捉解码或转换错误并打印
            except Exception as e:
                print(f"Unexpected error: {e}")  # 捕捉其他异常并打印

        self.save(history)  # 保存读取的数据
        return (
            history[-1] if history else None
        )  # 返回最后一条记录，如果没有记录返回None

    def save(self, history):
        now = datetime.now()  # 获取当前时间
        dir_path = "../10_tingml_datasets/"  # 设置保存文件的目录
        os.makedirs(dir_path, exist_ok=True)  # 创建目录，如果已存在则忽略
        file_name = os.path.join(
            dir_path, f"{self.direction}_{now.strftime('%Y-%m-%d_%H-%M-%S')}.npy"
        )  # 构建文件名
        np.save(file_name, np.array(history))  # 将历史记录保存为npy文件
        print(f"Data saved to {file_name}")  # 打印保存路径

    def __del__(self):
        if self.serial.isOpen():
            self.serial.close()  # 关闭串口连接
        print(f"Serial connection to {self.port} closed")  # 打印关闭连接信息


def main():
    # 创建 ArgumentParser 对象
    parser = argparse.ArgumentParser(description="指定运动状态")

    # 添加 方向 索引参数，使用 type=int 确保输入是整数
    parser.add_argument(
        "-d", "--direction", type=int, default=0, help="指定运动状态，默认为0,静止"
    )

    # 解析命令行参数
    args = parser.parse_args()
    _directions = ["Stationary_h","Stationary_v","Tilted", "Rotating", "Moving_h","Moving_v"]  # 定义方向选项
    if args.direction >= len(_directions):
        raise ValueError(
            f"args.direction should be less than num of directions: ({args.direction} >= {len(_directions)})"
        )
    control = SerialControl(
        direction=_directions[args.direction]
    )  # 创建SerialControl对象，设置方向为"Moving"
    control.get_log()  # 调用get_log方法读取数据


if __name__ == "__main__":
    main()  # 程序入口，调用main函数

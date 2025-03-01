from bluepy import btle
import time 
class ScanDelegate(btle.DefaultDelegate):
    def __init__(self):
        btle.DefaultDelegate.__init__(self)

    def handleDiscovery(self, dev, isNewDev, isNewData):
        pass
        # if isNewDev:
        #     print(f"Discovered device {dev.addr}")
        # elif isNewData:
        #     print(f"Received new data from {dev.addr}")

target_address = "08:a6:f7:b0:a0:51"
# target_address = "input your esp32 MAC addr, please convert captial letters to lowercase letter"
device_found = True
addr_type = "public"

def scan():
    global target_address, device_found, addr_type
    # 初始化扫描器并设置回调
    scanner = btle.Scanner().withDelegate(ScanDelegate())
    print("-----------------------------------------")
    print("Scanning for devices...")
    devices = scanner.scan(5)  # 扫描10秒

    device_found = False
    print("Scan complete. Found devices:")
    for dev in devices:
        print(f"Device {dev.addr} (type={dev.addrType}) (RSSI={dev.rssi}):")
        for (adtype, desc, value) in dev.getScanData():
            print(f"  {desc}: {value} ")
        if dev.addr == target_address:
            device_found = True
            addr_type = dev.addrType
            print("have found:", dir(dev))


def send():
    if device_found:
        print(f"Target device {target_address} found, attempting to connect...")
        try:
            # 连接到BLE设备
            device = btle.Peripheral(target_address, addr_type)

            # 获取服务和特性, 与C++里面是反过来的
            gatt_svr_svc_rw_demo_uuid = "1bce38b3-d137-48ff-a13e-033e14c7a315"
            service = device.getServiceByUUID(gatt_svr_svc_rw_demo_uuid)
            gatt_svr_chr_rw_demo_write_uuid = "35f28386-3070-4f3b-ba38-27507e991762"
            characteristic = service.getCharacteristics(gatt_svr_chr_rw_demo_write_uuid)[0]
            gatt_svr_chr_rw_demo_readonly_uuid = "16151413-1211-1009-0807-060504030201"
            only_read_characteristic = service.getCharacteristics(gatt_svr_chr_rw_demo_readonly_uuid)[0]
            print(f"Get Data: {only_read_characteristic.read().decode()}")
            # recv_data = only_read_characteristic.

            # 写数据到特性
            # data_to_write = "HELLO WORLD"
            # characteristic.write(data_to_write.encode('utf-8'), withResponse=False)
            # TURN ON LED
            data_to_write = bytes([0x1])
            characteristic.write(data_to_write, withResponse=False)
            print(f"Data '{data_to_write}' written to characteristic.")
            time.sleep(1)
            only_read_characteristic = service.getCharacteristics(gatt_svr_chr_rw_demo_readonly_uuid)[0]
            print(f"Get Data: {only_read_characteristic.read().decode()}")
            
            time.sleep(4)
            # TURN OFF LED
            
            data_to_write = bytes([0x0])
            rc = characteristic.write(data_to_write, withResponse=False)
            print(f"Data '{data_to_write}' written to characteristic.")
            time.sleep(1)
            only_read_characteristic = service.getCharacteristics(gatt_svr_chr_rw_demo_readonly_uuid)[0]
            print(f"Get Data: {only_read_characteristic.read().decode()}")
            # 断开连接
            device.disconnect()
            print("Disconnected from device.")
        except btle.BTLEException as e:
            print(f"Failed to connect or communicate with the device: {e}")
    else:
        print(f"Target device {target_address} not found.")
# scan()
send()

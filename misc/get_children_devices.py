from ctypes import *
cfg = windll.cfgmgr32

RERVALS = {
        0x00000000:"CR_SUCCESS",
        0x00000001:"CR_DEFAULT",
        0x00000002:"CR_OUT_OF_MEMORY",
        0x00000003:"CR_INVALID_POINTER",
        0x00000004:"CR_INVALID_FLAG",
        0x00000005:"CR_INVALID_DEVNODE",
        0x00000006:"CR_INVALID_RES_DES",
        0x00000007:"CR_INVALID_LOG_CONF",
        0x00000008:"CR_INVALID_ARBITRATOR",
        0x00000009:"CR_INVALID_NODELIST",
        0x0000000A:"CR_DEVNODE_HAS_REQS",
        0x0000000B:"CR_INVALID_RESOURCEID",
        0x0000000C:"CR_DLVXD_NOT_FOUND",   
        0x0000000D:"CR_NO_SUCH_DEVNODE",
        0x0000000E:"CR_NO_MORE_LOG_CONF",
        0x0000000F:"CR_NO_MORE_RES_DES",
        0x00000010:"CR_ALREADY_SUCH_DEVNODE",
        0x00000011:"CR_INVALID_RANGE_LIST",
        0x00000012:"CR_INVALID_RANGE",
        0x00000013:"CR_FAILURE",
        0x00000014:"CR_NO_SUCH_LOGICAL_DEV",
        0x00000015:"CR_CREATE_BLOCKED",
        0x00000016:"CR_NOT_SYSTEM_VM",   
        0x00000017:"CR_REMOVE_VETOED",
        0x00000018:"CR_APM_VETOED",
        0x00000019:"CR_INVALID_LOAD_TYPE",
        0x0000001A:"CR_BUFFER_SMALL",
        0x0000001B:"CR_NO_ARBITRATOR",
        0x0000001C:"CR_NO_REGISTRY_HANDLE",
        0x0000001D:"CR_REGISTRY_ERROR",
        0x0000001E:"CR_INVALID_DEVICE_ID",
        0x0000001F:"CR_INVALID_DATA",
        0x00000020:"CR_INVALID_API",
        0x00000021:"CR_DEVLOADER_NOT_READY",
        0x00000022:"CR_NEED_RESTART",
        0x00000023:"CR_NO_MORE_HW_PROFILES",
        0x00000024:"CR_DEVICE_NOT_THERE",
        0x00000025:"CR_NO_SUCH_VALUE",
        0x00000026:"CR_WRONG_TYPE",
        0x00000027:"CR_INVALID_PRIORITY",
        0x00000028:"CR_NOT_DISABLEABLE",
        0x00000029:"CR_FREE_RESOURCES",
        0x0000002A:"CR_QUERY_VETOED",
        0x0000002B:"CR_CANT_SHARE_IRQ",
        0x0000002C:"CR_NO_DEPENDENT",
        0x0000002D:"CR_SAME_RESOURCES",
        0x0000002E:"CR_NO_SUCH_REGISTRY_KEY",
        0x0000002F:"CR_INVALID_MACHINENAME",   
        0x00000030:"CR_REMOTE_COMM_FAILURE",   
        0x00000031:"CR_MACHINE_UNAVAILABLE",   
        0x00000032:"CR_NO_CM_SERVICES",   
        0x00000033:"CR_ACCESS_DENIED",   
        0x00000034:"CR_CALL_NOT_IMPLEMENTED",
        0x00000035:"CR_INVALID_PROPERTY",
        0x00000036:"CR_DEVICE_INTERFACE_ACTIVE",
        0x00000037:"CR_NO_SUCH_DEVICE_INTERFACE",
        0x00000038:"CR_INVALID_REFERENCE_STRING",
        0x00000039:"CR_INVALID_CONFLICT_LIST",
        0x0000003A:"CR_INVALID_INDEX",
        0x0000003B:"CR_INVALID_STRUCTURE_SIZE",
        0x0000003C:"NUM_CR_RESULTS"
    }

CM_DRP_DEVICEDESC = 1
CM_DRP_DRIVER = 0x0000A
NULL = 0

def get_dev_desc(devInst):
    buf = (c_wchar*1024)()
    blen = c_int(1024)
    cr = cfg.CM_Get_DevNode_Registry_PropertyW(devInst, CM_DRP_DEVICEDESC, NULL, buf, byref(blen), 0)
    if cr == 0:
        return buf.value
    else:
        return "ERR(%d):%s"%(devInst, RERVALS[cr])

def get_dev_id(devInst):
    buf = (c_wchar*1024)()
    blen = c_int(1024)
    cr = cfg.CM_Get_Device_IDW(devInst, buf, byref(blen), 0)
    if cr == 0:
        return buf.value
    else:
        return "ERR(%d):%s"%(devInst, RERVALS[cr])

drivers = []
def get_dev_driver(devInst):
    global drivers
    buf = (c_wchar*1024)()
    blen = c_int(1024)
    cr = cfg.CM_Get_DevNode_Registry_PropertyW(devInst, CM_DRP_DRIVER, NULL, buf, byref(blen), 0);
    if cr == 0:
        drivers.append(buf.value)
        return buf.value
    else:
        return "ERR(%d):%s"%(devInst, RERVALS[cr])

from xml.dom.minidom import *
def dev_xml():
    def dev_child(devInst, tree, lev, dom):
        devParent = c_int(devInst)
        devChild = c_int(0)
        devNextChild = c_int(0)
        if cfg.CM_Get_Child(byref(devChild), devParent, 0) == 0:
            desc = get_dev_desc(devChild.value)
            devId = get_dev_id(devChild.value)
            driver = get_dev_driver(devChild.value)
            node = dom.createElement("Device")
            node.setAttribute("DevInst", str(devChild.value))
            node.setAttribute("Desc", desc)
            node.setAttribute("Lev", str(lev))
            node.setAttribute("DevId", devId)
            node.setAttribute("Driver", driver)
            tree.appendChild(node)
            dev_child(devChild.value, node, lev + 1, dom)
            while cfg.CM_Get_Sibling(byref(devNextChild), devChild, 0) == 0:
                devChild.value = devNextChild.value
                desc = get_dev_desc(devChild.value)
                devId = get_dev_id(devChild.value)
                driver = get_dev_driver(devChild.value)
                node = dom.createElement("Device")
                node.setAttribute("DevInst", str(devChild.value))
                node.setAttribute("Desc", desc)
                node.setAttribute("Lev", str(lev))
                node.setAttribute("DevId", devId)
                node.setAttribute("Driver", driver)
                tree.appendChild(node)
                dev_child(devChild.value, node, lev + 1, dom)

    dom = Document()
    dom.appendChild(dom.createElement("DeviceTree"))
    devInst = c_int(0)
    devInstNext = c_int(0)
    lev = 0
    if 0 == cfg.CM_Locate_DevNodeW(byref(devInst), 0, 0):
        desc = get_dev_desc(devInst.value)
        devId = get_dev_id(devInst.value)
        driver = get_dev_driver(devInst.value)
        node = dom.createElement("Device")
        node.setAttribute("DevInst", str(devInst.value))
        node.setAttribute("Desc", desc)
        node.setAttribute("Lev", str(lev))
        node.setAttribute("DevId", devId)
        node.setAttribute("Driver", driver)
        dom.documentElement.appendChild(node)
        while 0 == cfg.CM_Get_Sibling(byref(devInstNext), devInst, 0):
            devInst.value = devInstNext.value
            desc = get_dev_desc(devInst.value)
            devId = get_dev_id(devInst.value)
            driver = get_dev_driver(devInst.value)
            node = dom.createElement("Device")
            node.setAttribute("DevInst", str(devInst.value))
            node.setAttribute("Desc", desc)
            node.setAttribute("Lev", str(lev))
            node.setAttribute("DevId", devId)
            node.setAttribute("Driver", driver)
            dom.documentElement.appendChild(node)
    for child in dom.documentElement.childNodes:
        k = int(child.getAttribute("DevInst"))
        dev_child(k, child, lev + 1, dom)
    return dom.toprettyxml()
import time
st = time.time()
xml = dev_xml()
et = time.time()
print("use time:", et - st)
#open("D:\\DeviceTree.xml", "wb").write(xml.encode("utf8"))
print(xml)


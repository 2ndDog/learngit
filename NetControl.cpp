// NetControl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <windows.h>
#include <fwpmu.h>
#include <iostream>
#include <string>

#pragma comment(lib, "fwpuclnt.lib")

using namespace std;

static int EnableNetControl(const wstring& appPath);

int main()
{
	// 手动输入应用程序路径(控制台输入GB18030)
	string strAppPath;
	cout << "请输入应用程序路径: " << endl;
	cin >> strAppPath;
	cout << strAppPath << endl;

	// 转换为宽字符串
	int len = MultiByteToWideChar(CP_ACP, 0, strAppPath.c_str(), -1, NULL, 0);
	wstring wstrAppPath(len, L'\0');
	MultiByteToWideChar(CP_ACP, 0, strAppPath.c_str(), -1, &wstrAppPath[0], len);

	// 使用WFP对指定程序强制联网
	int result = EnableNetControl(wstrAppPath);

	system("pause");
	return 0;
}

// 使用WFP对指定程序强制联网
static int EnableNetControl(const wstring& appPath)
{
	// 初始化防火墙策略存储的句柄
	HANDLE engineHandle = NULL;
	DWORD result = FwpmEngineOpen0(
		NULL,
		RPC_C_AUTHN_WINNT,
		NULL,
		NULL,
		&engineHandle
	);

	if (result != ERROR_SUCCESS)
	{
		cout << "无法打开防火墙引擎。错误代码: " << result << endl;
		return 1;
	}

	// 创建一个新的过滤规则
	FWPM_FILTER0 filter = { 0 };

	// 设置过滤规则的属性
	filter.displayData.name = _wcsdup(L"强制联网"); // 过滤规则名称
	filter.displayData.description = _wcsdup(L"允许应用所有连接"); // 过滤规则描述
	filter.layerKey = FWPM_LAYER_ALE_AUTH_CONNECT_V4; // 连接层
	filter.action.type = FWP_ACTION_PERMIT; // 允许连接
	filter.weight.type = FWP_EMPTY; // 不设置权重
	filter.filterCondition = NULL; // 不设置过滤条件, 允许所有出站连接
	filter.numFilterConditions = 0; // 过滤条件数量为0
	filter.subLayerKey = FWPM_SUBLAYER_UNIVERSAL; // 通用子层

	// 设置应用程序路径
	FWP_BYTE_BLOB* appID = nullptr;
	result = FwpmGetAppIdFromFileName0(appPath.c_str(), &appID);

	if (result != ERROR_SUCCESS)
	{
		cout << "无法获取应用程序ID。错误代码: " << result << endl;

		free(filter.displayData.name);
		free(filter.displayData.description);
		return 1;
	}

	// 设置过滤规则的条件
	filter.filterCondition = new FWPM_FILTER_CONDITION0[1];
	filter.filterCondition[0].fieldKey = FWPM_CONDITION_ALE_APP_ID; // 应用程序ID
	filter.filterCondition[0].matchType = FWP_MATCH_EQUAL; // 等于
	filter.filterCondition[0].conditionValue.type = FWP_BYTE_BLOB_TYPE; // 字节块
	filter.filterCondition[0].conditionValue.byteBlob = appID; // 应用程序ID值
	filter.numFilterConditions++; // 过滤条件数量加1

	// 添加过滤规则
	UINT64 filterId = 0;
	result = FwpmFilterAdd0(
		engineHandle,
		&filter,
		NULL,
		&filterId
	);

	if (result != ERROR_SUCCESS)
	{
		cout << "无法添加过滤规则。错误代码: " << result << endl;

		free(filter.displayData.name);
		free(filter.displayData.description);
		delete[] filter.filterCondition;
		return 1;
	}

	// 释放资源
	free(filter.displayData.name);
	free(filter.displayData.description);
	delete[] filter.filterCondition;
	FwpmFreeMemory0((void**)&appID);

	// 关闭防火墙引擎
	FwpmEngineClose0(engineHandle);

	cout << "已成功添加过滤规则。" << endl;
	return 0;
}

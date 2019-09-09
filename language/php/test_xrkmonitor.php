<?php 
    echo "test xrkmonitor\n";

	// 是否输出调试日志, 注意接口调用失败，失败信息在返回中，调试接口不会再输出日志
	// 调试日志路径：
	MtReport_phpex_set_debug(1);

	// 参数同接口 - MtReport_Init
	// 如需上报日志，首个参数为日志配置 id
	echo "test php_MtReport_Init\n";
    $ret = php_MtReport_Init();
	var_dump($ret);

	echo "test php_MtReport_Attr_Add\n";
	$ret = php_MtReport_Attr_Add(357, rand()%1000);
	var_dump($ret);

	echo "test php_MtReport_Attr_Set\n";
	$ret = php_MtReport_Attr_Set(358, 1235);
	var_dump($ret);
	if($ret["ret_code"] == 0) {
		$ret = php_MtReport_Attr_Set(358, 2235);
		var_dump($ret);
		$ret = php_MtReport_Attr_Set(358, 666);
		var_dump($ret);
	}


	// 字符串型监控点测试，注意字符串型监控点只在视图中展示
	// 如需查看上报图表，请先在视图中绑定字符串型监控点
	echo "test php_MtReport_Str_Attr_Add\n";
	$ret = php_MtReport_Str_Attr_Add(359, "rock123", 1234);
	var_dump($ret);
	if($ret["ret_code"] == 0) {
		$ret = php_MtReport_Str_Attr_Add(359, "hello123", 253);
		var_dump($ret);
		$ret = php_MtReport_Str_Attr_Add(359, "first123", 666);
		var_dump($ret);
	}

	echo "test php_MtReport_Str_Attr_Set\n";
	$ret = php_MtReport_Str_Attr_Set(360, "settest", 123);
	var_dump($ret);
	if($ret["ret_code"] == 0) {
		$ret = php_MtReport_Str_Attr_Set(360, "settest", 2123);
		var_dump($ret);
		$ret = php_MtReport_Str_Attr_Set(360, "settest", 423);
		var_dump($ret);
	}
	
    echo "\n";
?>



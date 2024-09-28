<!DOCTYPE html>
<?php
session_start();
if(empty($_SESSION['isLogin']))
{
echo "<script language='javascript'> window.location.href='index.php';</script>";
}
include_once "./private/Getdata.php";
?>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=gb2312" />
<title>充電樁設置</title>
<link href="../css/public.css?v=1.0" rel="stylesheet" type="text/css">
</head>
<script language="javascript">
function onCheckBoxClick(chkbox, shadow)
{
if(chkbox.checked ==true)
{
shadow.value ="1";
}
else
{
shadow.value ="0";
}
}
</script>
<body>
<div id="index" >
<div class="indexBox">
<div class="ipage">
<table width="800" border="0" align="center">
<tr>
<td>
<form method="post" name="form1" id="form1" action="./private/submit.php" enctype="multipart/form-data">
<table width="100%" border="1" cellpadding="1" cellspacing="1" class="tableSTYLE" background="images/_BK.jpg">
<tr>
<td height="35" colspan="10" align="center"><span class="thSTYLE">evMega</span></td>
</tr>
<tr>
<td width="100" height="35" align="center"></td>
<td width="75"  height="35" align="center"><a href="base_data.php" class="selectedSTYLE">基本</a></td>
<td width="75"  height="35" align="center"><a href="auth_net.php">授權</a></td>
<td width="75"  height="35" align="center"><a href="media_type.php">媒體</a></td>
<td width="75"  height="35" align="center"><a href="load_balance.php">負載平衡</a></td>
<td width="75"  height="35" align="center"><a href="IP_setting.php">IP設定</a></td>
<td width="75"  height="35" align="center"><a href="record_charging.php">本機數據</a></td>
<td width="75"  height="35" align="center"><a href="rader_setting.php">雷達</a></td>
<td width="75"  height="35" align="center"><a href="user_psw.php">本機用戶</a></td>
<td width="100" height="35" align="center"><a href="update_firmware.php">軟件更新</a></td>
</tr>
<tr>
<td colspan="1" height="360" align="center" valign="top">
<table width="100%" height="20" border="0" align="center" cellpadding="0" cellspacing="0">
<tr>
<td width="100%" height="35" align="left"><a href="#" class="selectedSTYLE">資料</a></td>
</tr>
<tr>
<td width="100%" height="2" class="line">
</tr>
<tr>
<td width="100%" height="35" align="left"><a href="base_electrical.php" class="indexSTYLE">電器特性</a></td>
</tr>
<tr>
<td width="100%" height="2" class="line">
</tr>
<tr>
<td width="100%" height="35" align="left"><a href="base_login.php" class="indexSTYLE">登入密碼</a></td>
</tr>
<tr>
<td width="100%" height="2" class="line">
</tr>
<tr>
<td width="100%" height="35" align="left"><a href="base_mode.php" class="indexSTYLE">操作模式</a></td>
</tr>
<tr>
<td width="100%" height="2" class="line">
</tr>
<tr>
<td width="100%" height="35" align="left"><a href="base_displayinfo.php" class="indexSTYLE">顯示信息</a></td>
</tr>
<tr>
<td width="100%" height="2" class="line">
</tr>
</table>
</td>
<td colspan="8" height="360" align="center" valign="top">
<table width="100%" height="30" border="0" align="center">
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> 登陸地址 </td>
<td width="390" height="30" align="left" class="editSTYLE"> <input name="webAddress" type="text" id="webAddress" size="40" maxlength="100" class="editSTYLE" value="<?php  echo GetItemValue('webAddress'); ?>" /> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> 充電樁編號 </td>
<td width="360" height="30" align="left" class="editSTYLE"> <input name="chargerNo" type="text" id="chargerNo" size="40" maxlength="100" class="editSTYLE" value="<?php  echo GetItemValue('chargerNo'); ?>" /> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> 充電樁型號 </td>
<td width="360" height="30" align="left" class="editSTYLE"> <input name="chargerType" type="text" id="chargerType" size="40" maxlength="100" class="editSTYLE" value="<?php  echo GetItemValue('chargerType'); ?>" /> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> 生產商 </td>
<td width="360" height="30" align="left" class="editSTYLE"> <input name="manufacturer" type="text" id="manufacturer" size="40" maxlength="100" class="editSTYLE" value="<?php  echo GetItemValue('manufacturer'); ?>" /> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> 出廠編號 </td>
<td width="360" height="30" align="left" class="editSTYLE"> <input name="chargerCode" type="text" id="chargerCode" size="40" maxlength="100" class="editSTYLE" value="<?php  echo GetItemValue('chargerCode'); ?>" /> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> 充電樁名稱 </td>
<td width="360" height="30" align="left" class="editSTYLE"> <input name="chargerName" type="text" id="chargerName" size="40" maxlength="100" class="editSTYLE" value="<?php  echo GetItemValue('chargerName'); ?>" /> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> RS485轉換器編號 </td>
<td width="360" height="30" align="left" class="editSTYLE"> <input name="serialNoA" type="text" id="serialNoA" size="40" maxlength="100" class="editSTYLE" value="<?php  echo GetItemValue('serialNoA'); ?>" /> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> 車輛距離閾值 </td>
<td width="360" height="30" align="left" class="editSTYLE"> <input name="DistanceThreshold" type="text" id="DistanceThreshold" size="40" maxlength="100" class="editSTYLE" value="<?php  echo GetItemValue('DistanceThreshold'); ?>" /> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> 距離感知次數 </td>
<td width="360" height="30" align="left" class="editSTYLE"> <input name="DistanceSenseTimes" type="text" id="DistanceSenseTimes" size="40" maxlength="100" class="editSTYLE" value="<?php  echo GetItemValue('DistanceSenseTimes'); ?>" /> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
<td width="180" height="30" align="left" class="tbLabelSTYLE"> 是網路版 </td>
<input name="NETVER" type="hidden" id="NETVER" value="<?php  $tmp=GetItemValue('NETVER'); echo $tmp; ?>" />
<td width="360" height="30" align="left" class="editSTYLE"> <input name="NETVER_" type="checkbox" id="NETVER_" size="40" maxlength="100" class="checkboxSTYLE" onClick="onCheckBoxClick(this, NETVER)" <?php if($tmp=="1") echo "checked"?>  /> </td>
</tr>
<tr>
<td width="30" height="30" align="left" class="tbLabelSTYLE"> </td>
</tr>
<tr>
<td colspan="3" height="60" align="right" valign="bottom" class="tbLabelSTYLE" style="padding-right:60px;padding-bottom:5px;"> <input type="reset" name="clean" value="重置" id="btn" class="btnSTYLE"/>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<input type="submit" name="Submit" value="確認" id="btn" class="btnSTYLE"/> </td>
</tr>
</table>
</td>
<td colspan="1" height="360" align="center" valign="top">
<table width="100%" height="100%" border="0" align="center">
</table>
</td>
</tr>
<tr>
</tr>
</table>
</form>
</td>
</tr>
</table>
</div>
</div>
</div>
</body>
</html>

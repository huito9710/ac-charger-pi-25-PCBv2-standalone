<!DOCTYPE html>
<html>
<?php
include_once "./private/Getdata.php";
?>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=gb2312" />
<title>Login <?php $chargerCode=GetItemValue('chargerCode'); echo $chargerCode; ?></title>
<meta name="charger" content=<?php echo $chargerCode; ?> />
<link href="../css/public.css?v=1.0" rel="stylesheet" type="text/css">
</head>
<script language="javascript">
function Checkblank(form)
{
if(form.user.value=="")
{
alert("Please input user name"); form.user.focus(); return false;
}
if(form.pwd.value=="")
{
alert("Please input password"); form.pwd.focus(); return false;
}
form.submit();
return true;
}
function onKeyDown(form, value)
{
var str ="";
if(form.focusFlag.value =='1')
{
str =form.user.value;
if(str.length <15)
{
form.user.value =str +value;
}
}
else
{
str =form.pwd.value;
if(str.length <12)
{
form.pwd.value =str +value;
}
}
return true;
}
function IsUserBox(form, value)
{
form.focusFlag.value =value;
}
</script>
<body>
<div id="index" >
<div class="indexBox">
<div class="ipage">
<table width="800" border="0" align="center">
<tr>
<td>
<form method="post" name="form1" id="form1" action="./private/checkuser.php" enctype="multipart/form-data">
<table width="100%" border="0" cellpadding="1" cellspacing="1" class="tableSTYLE" background="images/_BK.jpg">
<tr>
<td height="40" align="center" style="padding-left:100px;"><span class="thSTYLE">evMega</span></td>
<td width="100" height="40" align="right"><a href="../ZH/index.php" class="selectedSTYLE">中文</a></td>
</tr>
<tr>
<td colspan="2" height="2" class="line">
</tr>
<tr>
<td height="30" align="left" class="tbLabelSTYLE"> </td>
</tr>
<tr>
<td colspan="2" height="100" align="center" valign="middle">
<table width="35%" height="100%" border="0" align="center" cellpadding="1" cellspacing="1">
<tr>
<input name="focusFlag" type="hidden" id="focusFlag" value="1" />
<td width="50%" height="30" align="left" class="tbLabelSTYLE"> User Name </td>
<td width="50%" height="30" align="left" class="editSTYLE"> <input name="user" type="text" id="user" size="16" maxlength="20" class="editSTYLE" value="" style="width:100%;" onfocus="return IsUserBox(form1, '1');"/> </td>
</tr>
<tr>
<td width="50%" height="30" align="left" class="tbLabelSTYLE"> Password </td>
<td width="50%" height="30" align="left" class="editSTYLE"> <input name="pwd" type="password" id="pwd" size="16" maxlength="12" class="editSTYLE" value="" style="width:100%;" onfocus="return IsUserBox(form1, '0');"/> </td>
</tr>
</table>
</td>
</tr>
<tr>
<td colspan="2" height="260" align="center" valign="middle">
<table width="40%" height="80%" border="0" cellpadding="1" cellspacing="3" align="center">
<tr>
<td width="25%" height="33%" align="center" valign="middle"> <input type="button" name="n1" value="1" id="btn" class="numBtnSTYLE" onClick="return onKeyDown(form1, this.value);"/> </td>
<td width="25%" height="33%" align="center" valign="middle"> <input type="button" name="n2" value="2" id="btn" class="numBtnSTYLE" onClick="return onKeyDown(form1, this.value);"/> </td>
<td width="25%" height="33%" align="center" valign="middle"> <input type="button" name="n3" value="3" id="btn" class="numBtnSTYLE" onClick="return onKeyDown(form1, this.value);"/> </td>
<td width="25%" height="33%" align="center" valign="middle"> <input type="reset" name="clean" value="Reset" id="btn" class="numBtnSTYLE" style="background-color:#D1322D;"/> </td>
</tr>
<tr>
<td width="25%" height="33%" align="center" valign="middle"> <input type="button" name="n4" value="4" id="btn" class="numBtnSTYLE" onClick="return onKeyDown(form1, this.value);"/> </td>
<td width="25%" height="33%" align="center" valign="middle"> <input type="button" name="n5" value="5" id="btn" class="numBtnSTYLE" onClick="return onKeyDown(form1, this.value);"/> </td>
<td width="25%" height="33%" align="center" valign="middle"> <input type="button" name="n6" value="6" id="btn" class="numBtnSTYLE" onClick="return onKeyDown(form1, this.value);"/> </td>
<td width="25%" height="33%" align="center" valign="middle"> <input type="submit" name="Submit" value="Enter" id="btn" class="numBtnSTYLE" onClick="return Checkblank(form1);" style="background-color:#008080;"/> </td>
</tr>
<tr>
<td width="25%" height="33%" align="center" valign="middle"> <input type="button" name="n7" value="7" id="btn" class="numBtnSTYLE" onClick="return onKeyDown(form1, this.value);"/> </td>
<td width="25%" height="33%" align="center" valign="middle"> <input type="button" name="n8" value="8" id="btn" class="numBtnSTYLE" onClick="return onKeyDown(form1, this.value);"/> </td>
<td width="25%" height="33%" align="center" valign="middle"> <input type="button" name="n9" value="9" id="btn" class="numBtnSTYLE" onClick="return onKeyDown(form1, this.value);"/> </td>
<td width="25%" height="33%" align="center" valign="middle"> <input type="button" name="n0" value="0" id="btn" class="numBtnSTYLE" onClick="return onKeyDown(form1, this.value);"/> </td>
</tr>
</table>
</td>
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

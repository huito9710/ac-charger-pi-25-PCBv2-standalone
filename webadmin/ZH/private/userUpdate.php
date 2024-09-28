<?php
session_start();
if(empty($_SESSION['isLogin']))
{
echo "<script language='javascript'> window.location.href='../index.php';</script>";
}
include_once "./Savedata.php";
if(isset($_POST['pswList']))
{
SaveFileText("password.lst", "pswList");
}
else if(isset($_POST['rfidList']))
{
SaveFileText("Card.lst", "rfidList");
}
echo "<script language='javascript'>; history.back(-1); </script>";
?>

<!DOCTYPE html>
<html>
<head>
<title>Border Tabs - Css</title>
<meta http-equiv="content-type" content="text/html; charset=iso-8859-1">
<meta name="generator" content="HAPedit 3.0">
<style type="text/css">
html,body{margin:0;padding:0}
body{font: 100.01% "Trebuchet MS",Arial,sans-serif}
div#header{background-color:#9CF}
div#header h1{margin:0;line-height:70px;margin-left:20px}

div#navigation{background-color: #9cf;border-bottom: 1px solid #787878;padding-left: 20px}
div#navigation ul{list-style-type: none;margin: 0;padding: 0;white-space: nowrap}
div#navigation li{display: inline;margin: 0;padding: 0}
div#navigation li a{text-decoration: none;border: 1px solid #787878;padding: 0px 0.3em;
    background: #ccc;color: #036}
div#navigation li a:hover{background-color: #f0f0f0}
div#navigation li#activelink a{border-bottom: 1px solid #fff;background-color: #fff;color: #603}
</style>
</head>
<body>
<div id="header"><h1></h1></div>
<div id="navigation">
<ul>
   
    <li><a href="fpgaconfig.php">FPGAconfig</a></li>
    <li><a href="miniDAQ.php">MiniDAQ</a></li>
    <li><a href="datatest.php">Datatest</a></li>
    <li id="activelink"><a href="calibration.php">Calibration</a></li>
</ul>
</div>
<form action="action_calibration.php" method=post>
<br><br><br><br>
<input type="radio" name="type" value="fast" checked>Fast Calibration
<br><br>
<input type="radio" name="type" value="old">Old Calibration
<br><br>
<input type="checkbox" name="ScanVPlus" value="yes" checked>Scan Vcth vs VPlus
<br><br>
<input type="checkbox" name="Bitwise" value="yes">Bitwise Offset Tuning
<br><br>
<input type="checkbox" name="All_channel" value="yes">Calibration using all channel
<br><br>
Output Folder:<input type="textfield" name="output" value="Results">
<br><br>
Hw Description File:<input type="textfield" name="Hw Description File " value="settings/Calibration2CBC.xml">
<br><br><br><br>
<input type="submit" value="Submit">
</form> 
</body>
</html>

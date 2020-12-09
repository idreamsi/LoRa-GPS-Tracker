<?php
/*
   This file is part of Project LoRa GPS Tracker with Wio Terminal
   https://www.hackster.io/idreams/
   Edited by: Ramin Sangesari
   Thanks to Mohamed Fadiga for this code.
*/   
   	function getParameter($par, $default = null)
   	{
   		if (isset($_POST[$par]) && strlen($_POST[$par])) return $_POST[$par];
   		if (isset($_GET[$par]) && strlen($_GET[$par])) return $_GET[$par];
   		else return $default;
   	}        
   	$param_names = array("date", "time","lat","lon");
   	$file = getParameter("date").".txt";
   	$line="";
   	for($i=0; $i< count($param_names); $i++)
   	{
   		$line = $line.getParameter($param_names[$i]);
   		if($i<count($param_names)-1)$line = $line.",";
   	}
   	$line = $line."\n";
   	file_put_contents($file, $line, FILE_APPEND | LOCK_EX);
   ?>

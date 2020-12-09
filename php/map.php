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
   ?>
<!DOCTYPE html>
<html>
   <head>
      <script language="JavaScript" type="text/javascript" src="jquery-2.1.4.min.js"></script>     
      <script src="http://maps.google.com/maps/api/js?key=YOUR_API_KEY&libraries=geometry"></script>
      <script type="text/javascript">
         var map;
         var poly;
         var file_name = <?php echo "\"".getParameter("file")."\"" ?>;//get title name
         var lastLat=0;
         var lastLon=0;
         //-------------------
         var firstLat = 0;
         var firstLon = 0;
         var calc;
         //-------------------
         var polyOptions= //polyline properties
         {
         	strokeColor: '0#000000',
         	strokeOpacity: 1.0,
         	strokeWeight: 3
         };
         var mapProp = //map properties
         {
         	zoom:15,
         	mapTypeId:google.maps.MapTypeId.ROADMAP
         };
                  
         //calculates distance between two points in km's
         function calcDistance(p1, p2) {
         	return (google.maps.geometry.spherical.computeDistanceBetween(p1, p2) / 1000).toFixed(2);		
         }
            
         //   
         function haversineDistance(latlngA, latlngB) {
         const squared = x => x * x;
         const toRad = x => (x * Math.PI) / 180;
         const R = 6371; // Earth’s mean radius in km
         
         const dLat = toRad(latlngB[0] - latlngA[0]);
         const dLon = toRad(latlngB[1] - latlngA[1]);
         
         const dLatSin = squared(Math.sin(dLat / 2));
         const dLonSin = squared(Math.sin(dLon / 2));
         
         const a = dLatSin +
         		  (Math.cos(toRad(latlngA[0])) * Math.cos(toRad(latlngB[0])) * dLonSin);
         const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
         let distance = R * c;
         
         //if (isMiles) distance /= 1.609344;
         
         return distance;
          }
         
            
            
         function initialize()
         {
         	map=new google.maps.Map(document.getElementById("map"),mapProp);
         	poly=new google.maps.Polyline(
                    {
                      map:map,
                      icons: [{
                                icon: {
                                        path: google.maps.SymbolPath.FORWARD_CLOSED_ARROW,
                                        strokeColor:'#ff0066',
                                        fillColor:'#ff0066',
                                        fillOpacity:1
                                      },
                                repeat:'100px', //repeat polyline
                                path:[]
                             }]
                       });
         	poly.setMap(map);
         	$.get(file_name, function(txt) 
         	{
         		var c = 0; // counter for marker
                      	//console.log("file")
                      	var lines = txt.split("\n");
                      	for (var i=0;i<(lines.length-1);i++)
                       {
                  			var coords=lines[i].split(",");
                              if(lastLat!=coords[2]||lastLon!=coords[3])
                              {
                              	var pos = new google.maps.LatLng(coords[2],coords[3]);
         				poly.getPath().push(pos);
                                  map.setCenter(pos);
                                  lastLat = coords[2];
                                  lastLon = coords[3];
         			
         
         			if (c == 0 ) {
         				firstLat = lastLat; 
         				firstLon = lastLon;
         				c = 1;
         			}
         			var coords2 = lines[i+1].split(",");
         			if(lastLat!=coords2[2]||lastLon!=coords2[3])
                              {		
         										
         				var p1 = new google.maps.LatLng(lastLat, lastLon);
         				var p2 = new google.maps.LatLng(coords2[2], coords2[3]);
         				var cc = haversineDistance(p1, p2);
         			}	
         		
         			}	
                          }
         		var pA = new google.maps.LatLng(firstLat, firstLon);
         		var pB = new google.maps.LatLng(lastLat, lastLon);
         		var resultN = calcDistance(pA, pB);
         		//alert(resultN);
                    	});  
         	
         }
         
                  google.maps.event.addDomListener(window, 'load', initialize);
         
      </script>
   </head>
   <body >
      <div>
         <center>
            <div id="map" style="width: 100vw;height:100vh;"></div>
         </center>
      </div>
   </body>
</html>

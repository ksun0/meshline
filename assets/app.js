var mapContainer = document.getElementById('map-container');

var platform = new H.service.Platform({
  app_id: 'mXa5LbPjF3kscwDVP0iH', // // <-- ENTER YOUR APP ID HERE
  app_code: 'E2OGzWH1fAboRRlJztPMTg', // <-- ENTER YOUR APP CODE HERE
});

var coordinates = {
  lat: 39.9522,
  lng: -75.1932
};

var mapOptions = {
  center: coordinates,
  zoom: 14
}

// Pro tip: You can set the map's center pointer and zoom level using map.setCenter and map.setZoom methods at any point in your code.

var defaultLayers = platform.createDefaultLayers();

var map = new H.Map(
  mapContainer,
  defaultLayers.normal.map,
  mapOptions);


window.addEventListener('resize', function () {
  map.getViewPort().resize();
});


var behavior = new H.mapevents.Behavior(new H.mapevents.MapEvents(map));

var iconUrl = 'assets/marker-computer.png';

var iconOptions = {
	// The icon's size in pixel:
  size: new H.math.Size(36, 36),
	// The anchorage point in pixel,
	// defaults to bottom-center
  anchor: new H.math.Point(18, 36)
};

var markerOptions = {
   icon: new H.map.Icon(iconUrl, iconOptions)
};

// User location via browser's geolocation API
function updatePosition (event) {
  var coordinates = {
    lat: event.coords.latitude,
    lng: event.coords.longitude
  };

  // Add a new marker every time the position changes
  var marker = new H.map.Marker(coordinates, markerOptions);
  // Pro tip: While map.addObject adds new markers, you can remove them again using map.removeObject.\
  map.addObject(marker);
  // Recenter the map every time the position chnages
  map.setCenter(coordinates);
}

navigator.geolocation.watchPosition(updatePosition);



var marker = new H.map.Marker({lat: 39.96, lng: -75.20},
  { icon: new H.map.Icon('assets/marker-phone.png',
    {size: new H.math.Size(36, 36), anchor: new H.math.Point(18, 36) })
  });

map.addObject(marker);

var messages = [['please answer me @ben', 39.95, -75.19], ['hello', 39.947, -75.188]];

var funcs = [];

function createfunc(i) {
    return function() {
      $.ajax({
          type:"GET",
          url: "http://localhost:8000/predict/" + messages[i][0],
          success: function(data){
            console.log(i)
            console.log(messages[i][0])
            console.log(data)

            var marker = new H.map.Marker({lat: messages[i][1], lng: messages[i][2]},
              { icon: new H.map.Icon('assets/marker-help.png',
                {size: new H.math.Size(100, 60), anchor: new H.math.Point(50, 60) })
              });

            console.log(marker)

            map.addObject(marker);
          }
      });
    };
}

for (var i = 0; i < messages.length; i++) {
    funcs[i] = createfunc(i);
}

for (var j = 0; j < messages.length; j++) {
    funcs[j]();                        // and now let's run each one to see
}

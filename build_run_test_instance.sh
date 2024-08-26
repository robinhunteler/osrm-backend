# build
mkdir -p build
cd build
cmake ..
cmake --build .
sudo cmake --build . --target install
cd ..

# download
wget http://download.geofabrik.de/europe/germany/bremen-latest.osm.pbf

# create osrm data
osrm-extract -p profiles/car.lua bremen-latest.osm.pbf || echo "osrm-extract failed"
osrm-contract bremen-latest.osrm || echo "osrm-contract failed"

# run
osrm-routed --algorithm ch bremen-latest.osrm
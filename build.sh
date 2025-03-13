PROJ_NAME=Gunslinger

cmake -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -B build
if [ $? -ne 0 ]; then
	echo "Failed to configure $PROJ_NAME"
fi

cmake --build build
if [ $? -eq 0 ]; then
	cp "./build/bin/$PROJ_NAME" "./product/bin/$PROJ_NAME"
	cd ./product/bin
	./$PROJ_NAME
	cd ../..
else
	echo "Error, Failed to build $PROJ_NAME"
fi
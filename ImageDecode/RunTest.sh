#! /bin/sh
mkdir -p build
cd build && cmake .. && make
cd ..
rm -rf ./example_data/decoded*.jpeg
./build/hist_thresh ./example_data/all_leds.jpg ./example_data/bits.json
#check if file exists
if [ ! -e "./example_data/decoded_262143.jpg" ]; then
    echo "All led failed"
    exit 1
else
    echo "OK"
fi

rm -rf ./example_data/decoded*.jpeg


./build/hist_thresh ./example_data/random_3640.000000.jpg ./example_data/bits.json

if [ ! -e "./example_data/decoded_36400.jpg" ]; then
    echo "random_3640.000000 led failed"
    exit 1
else
    echo "OK"
fi

rm -rf ./example_data/decoded*.jpeg

./build/hist_thresh ./example_data/random_6286.000000.jpg ./example_data/bits.json

if [ ! -e "./example_data/decoded_62860.jpg" ]; then
    echo "random_3640.000000 led failed"
    exit 1
else
    echo "OK"
fi

rm -rf ./example_data/decoded*.jpeg

./build/hist_thresh ./example_data/ts_2.700000.jpg ./example_data/bits.json

if [ ! -e "./example_data/decoded_27.jpg" ]; then
    echo "ts_2.800000  failed"
    exit 1
else
    echo "OK"
fi

rm -rf ./example_data/decoded*.jpeg

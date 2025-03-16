#! /bin/sh
mkdir -p build
cd build && cmake .. && make
cd ..
rm -rf ./example_data_gray/decoded*.jpeg

./build/hist_thresh ./example_data_gray/random_231.000000.jpg ./example_data_gray/bits.json 

if [ ! -e "./example_data_gray/decoded_2310.jpg" ]; then
    echo "random_2310.000000 led failed"
    exit 1
else
    echo "OK"
fi

rm -rf ./example_data_gray/decoded*.jpeg

./build/hist_thresh ./example_data_gray/random_5155.000000.jpg ./example_data_gray/bits.json 

if [ ! -e "./example_data_gray/decoded_51550.jpg" ]; then
    echo "random_5155.000000 led failed"
    exit 1
else
    echo "OK"
fi
rm -rf ./example_data_gray/decoded*.jpeg

./build/hist_thresh ./example_data_gray/random_5418.000000.jpg ./example_data_gray/bits.json 

if [ ! -e "./example_data_gray/decoded_54180.jpg" ]; then
    echo "random_5418.000000 led failed"
    exit 1
else
    echo "OK"
fi
rm -rf ./example_data_gray/decoded*.jpeg

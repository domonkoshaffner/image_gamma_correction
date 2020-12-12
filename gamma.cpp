#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <CL/cl2.hpp>

#include <chrono>
#include <numeric>
#include <iterator>

#include <vector>       // std::vector
#include <string>
#include <exception>    // std::runtime_error, std::exception
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <random>       // std::default_random_engine, std::uniform_real_distribution
#include <algorithm>    // std::transform
#include <cstdlib>      // EXIT_FAILURE
#include <iomanip>
#include <math.h>
#include <windows.h>

struct rawcolor{ unsigned char r, g, b, a; };
struct rawcolor3{ unsigned char r, g, b; };
struct color   { float         r, g, b, a; };

int main()
{
    try
    {
        // Checking the device info
        cl::CommandQueue queue = cl::CommandQueue::getDefault();
        cl::Device device = queue.getInfo<CL_QUEUE_DEVICE>();
        cl::Context context = queue.getInfo<CL_QUEUE_CONTEXT>();

        // Load program source
        std::ifstream file2{ "C:/Users/haffn/Desktop/MSc-III/GPU-II/Projects/second_project_gamma/gamma.cl" };

        // if program failed to open
        if (!file2.is_open())
            throw std::runtime_error{ std::string{ "Cannot open kernel source: " } + "gamma.cl" };

        // Creating the program
        cl::Program program{ std::string{ std::istreambuf_iterator<char>{ file2 }, std::istreambuf_iterator<char>{} } };

        //Building the program
        program.build({ device });

        // Creating the kernel
        cl:: Kernel gamma(program, "gamma");

// ############################################################################
// Getting the degree of angle 
        
        float gamma_val = 0.5f;
        //std::cout << "Please enter the angle of rotation in degrees! \n";
        //std::cin >> angle;
 
// Getting the name of the input file:

        static const std::string input_filename = "C:/Users/haffn/Desktop/MSc-III/GPU-II/Projects/second_project_gamma/nice.jpg";

        int height = 0;
        int width = 0;
        int chanels = 0;

// Loading the image:

        rawcolor* data0 = reinterpret_cast<rawcolor*>(stbi_load(input_filename.c_str(), &width, &height, &chanels, 4 /* we expect 4 components */));
        if(!data0)
        {
            std::cout << "Error: could not open input file: " << input_filename << "\n";
            return -1;
        }
        else
        {
            std::cout << std::endl << "Image opened successfully." << "\n" << "\n";
        }

        std::vector<color> input(width*height);

// Transfrming the image according to the channels

        if(chanels == 4)
        {
            std::transform(data0, data0+width*height, input.begin(), [](rawcolor c){ return color{c.r/255.0f, c.g/255.0f, c.b/255.0f, c.a/255.0f}; } );
        }
        else if(chanels == 3)
        {
            std::transform(data0, data0+width*height, input.begin(), [](rawcolor c){ return color{c.r/255.0f, c.g/255.0f, c.b/255.0f, 1.0f}; } );
        }
        stbi_image_free(data0);

// ############################################################################ 
// Declaring some variables

        size_t width_size = width;
        size_t height_size = height;

// ############################################################################
// Crating the texture vector

        std::vector<cl::Image2D> textures(2);

// Creating the formats and declaring the image channel types

        cl::ImageFormat inFormat;
        cl::ImageFormat outFormat;

        inFormat.image_channel_data_type = CL_FLOAT;
        outFormat.image_channel_data_type = CL_FLOAT;

        inFormat.image_channel_order = CL_RGBA;
        outFormat.image_channel_order = CL_RGBA;

// Creating the textures with the appropriate data

        std::vector<color> output(width*height);

        textures[0] = cl::Image2D(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, inFormat, width_size, height_size, 0, input.data(), nullptr);
        textures[1] = cl::Image2D(context, CL_MEM_WRITE_ONLY, outFormat, width_size, height_size, 0, nullptr, nullptr);

        cl::Sampler sampler = cl::Sampler(context, CL_FALSE, CL_ADDRESS_CLAMP, CL_FILTER_NEAREST);

        std::array<cl::size_type, 3> origin = {0, 0, 0};
        std::array<cl::size_type, 3> dims = {width_size, height_size, 1};

// ############################################################################
// Setting the argument of the kernel

        gamma.setArg(0, textures[0]);
        gamma.setArg(1, textures[1]);
        gamma.setArg(2, sampler);
        gamma.setArg(3, gamma_val);

// Calling the kernel

        queue.enqueueNDRangeKernel(gamma, cl::NullRange, cl::NDRange(width_size, height_size));
        
        cl::finish();

        queue.enqueueReadImage(textures[1], CL_TRUE, origin, dims, 0, 0, output.data(), 0, nullptr);

// ############################################################################
// Creating the output data and saving the image

        std::vector<rawcolor> tmp(width*height*4);
        std::transform(output.cbegin(), output.cend(), tmp.begin(), 
        [](color c) {return rawcolor{ (unsigned char)(c.r*255.0f),
                                      (unsigned char)(c.g*255.0f),
                                      (unsigned char)(c.b*255.0f),
                                      (unsigned char)(1.0f*255.0f) }; });

        int res = stbi_write_png("result.png", width, height, 4, tmp.data(), width*4);
        if (res == 0)
        {
            std::cout << "Error writing utput to file\n";
        }
        else
        {
            std::cout << "Output written to file\n";
        }        

// ############################################################################
// If kernel failed to build

    }
    catch (cl::BuildError& error) 
    {
        std::cerr << error.what() << "(" << error.err() << ")" << std::endl;

        for (const auto& log : error.getBuildLog())
        {
            std::cerr <<
                "\tBuild log for device: " <<
                log.first.getInfo<CL_DEVICE_NAME>() <<
                std::endl << std::endl <<
                log.second <<
                std::endl << std::endl;
        }

        std::exit(error.err());
    }
    catch (cl::Error& error) // If any OpenCL error occurs
    {
        std::cerr << error.what() << "(" << error.err() << ")" << std::endl;
        std::exit(error.err());
    }
    catch (std::exception& error) // If STL/CRT error occurs
    {
        std::cerr << error.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }
}


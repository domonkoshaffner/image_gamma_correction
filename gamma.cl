__kernel void gamma(read_only image2d_t orig_im, write_only image2d_t new_im, sampler_t grid_sampler, float gamma_val) 
{

// Saving the global ids in variables

    int i = get_global_id(0);
    int j = get_global_id(1);

// Creating the 2D image objects
// Saving the original coordinates in the new ones

    float4 pixel = read_imagef(orig_im, grid_sampler, (int2)(i,j));
    write_imagef(new_im, (int2)(i,j), pow(pixel, gamma_val));
    
}

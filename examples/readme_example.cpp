#include <selene/img/Algorithms.hpp>
#include <selene/img/ImageConversions.hpp>
#include <selene/img/ImageDataToImage.hpp>
#include <selene/img/ImageToImageData.hpp>
#include <selene/img/IO.hpp>
#include <selene/img/Transformations.hpp>

#include <selene/io/FileUtils.hpp>

#include <boost/filesystem.hpp>

#include <cassert>

#include "Utils.hpp"

using namespace sln;  // Never outside of example code

int main(int argc, char** argv)
{
  // Read data path as optional command line argument
  const char* data_path = (argc >= 2) ? argv[1] : nullptr;

  const auto example_img_path = sln_examples::full_data_path("bike_duck.jpg", data_path);

  // Decode JPEG image data from disk
  ImageData<> img_data = read_image(FileReader(example_img_path.string()));
  assert(img_data.nr_channels() == 3 && img_data.nr_bytes_per_channel() == 1);

  // Convert to strongly typed RGB image
  Image<Pixel_8u3> img_rgb = to_image<Pixel_8u3>(std::move(img_data));
  assert(img_rgb.width() > 400_px && img_rgb.height() > 350_px);

  // Create non-owning view on part of the image
  Image<Pixel_8u3> img_part = view(img_rgb, 100_px, 100_px, 300_px, 250_px);

  // Darken this part
  for_each_pixel(img_part, [](auto& px){ px /= 4; });

  // Flip this part horizontally
  flip_horizontally_in_place(img_part);

  // Convert whole image to RGBA, adding semi-transparent alpha channel
  const Image<Pixel_8u4> img_rgba =
      convert_image<PixelFormat::RGB, PixelFormat::RGBA>(img_rgb, std::uint8_t{128});

  // Encode in-memory to PNG
  std::vector<std::uint8_t> encoded_png_data;
  write_image(to_image_data_view(img_rgba, PixelFormat::RGBA), ImageFormat::PNG,
              VectorWriter(encoded_png_data));

  // Write encoded binary data to disk (or do something else with it...)
  write_data_contents("example_out.png", encoded_png_data);
}
#if defined(_MSC_VER) && !defined(_WIN64)
#pragma warning(disable : 4244) // boost::number_distance::distance()
// converts 64 to 32 bits integers
#endif
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Classification.h>
#include <CGAL/bounding_box.h>
#include <CGAL/tags.h>
#include <CGAL/IO/read_points.h>
#include <CGAL/IO/read_las_points.h>
#include <CGAL/IO/write_ply_points.h>
#include <CGAL/IO/write_las_points.h>
#include <CGAL/Real_timer.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
typedef CGAL::Parallel_if_available_tag Concurrency_tag;
typedef CGAL::Simple_cartesian<double> Kernel;
typedef Kernel::Point_3 Point;
typedef Kernel::Iso_cuboid_3 Iso_cuboid_3;
typedef std::vector<Point> Point_range;
typedef CGAL::Identity_property_map<Point> Pmap;
namespace Classification = CGAL::Classification;
typedef Classification::Sum_of_weighted_features_classifier Classifier;
typedef Classification::Planimetric_grid<Kernel, Point_range, Pmap> Planimetric_grid;
typedef Classification::Point_set_neighborhood<Kernel, Point_range, Pmap> Neighborhood;
typedef Classification::Local_eigen_analysis Local_eigen_analysis;
typedef Classification::Label_handle Label_handle;
typedef Classification::Feature_handle Feature_handle;
typedef Classification::Label_set Label_set;
typedef Classification::Feature_set Feature_set;
typedef Classification::Feature::Distance_to_plane<Point_range, Pmap> Distance_to_plane;
typedef Classification::Feature::Elevation<Kernel, Point_range, Pmap> Elevation;
typedef Classification::Feature::Vertical_dispersion<Kernel, Point_range, Pmap> Dispersion;

void write_ply(
	std::vector<Point>& pts,
	const std::string& fileName,
	const Label_handle& ground,
	const Label_handle& vegetation,
	const Label_handle& roof,
	const std::vector<int>& label_indices,
	const Label_set& labels)
{

	std::vector<unsigned char> red, green, blue;
	red.reserve(pts.size());
	green.reserve(pts.size());
	blue.reserve(pts.size());
	for (std::size_t i = 0; i < pts.size(); ++i)
	{
		Label_handle label = labels[static_cast<std::size_t>(label_indices[i])];
		unsigned r = 0, g = 0, b = 0;
		if (label == ground)
		{
			r = 245;
			g = 180;
			b = 0;
		}
		else if (label == vegetation)
		{
			r = 0;
			g = 255;
			b = 27;
		}
		else if (label == roof)
		{
			r = 255;
			g = 0;
			b = 170;
		}
		red.push_back(r);
		green.push_back(g);
		blue.push_back(b);
	}
	std::ofstream f(fileName, std::ios::binary);
	CGAL::IO::set_binary_mode(f);

	CGAL::IO::write_PLY_with_properties(f, CGAL::make_range(boost::counting_iterator<std::size_t>(0), boost::counting_iterator<std::size_t>(pts.size())),
		CGAL::make_ply_point_writer(CGAL::make_property_map(pts)),
		std::make_pair(CGAL::make_property_map(red), CGAL::PLY_property<unsigned char>("red")),
		std::make_pair(CGAL::make_property_map(green), CGAL::PLY_property<unsigned char>("green")),
		std::make_pair(CGAL::make_property_map(blue), CGAL::PLY_property<unsigned char>("blue")));
	f.close();
}

void write_las(
	std::vector<Point>& pts,
	const std::string& fileName,
	const Label_handle& ground,
	const Label_handle& vegetation,
	const Label_handle& roof,
	const std::vector<int>& label_indices,
	const Label_set& labels)
{

	std::vector<unsigned char> red, green, blue;
	red.reserve(pts.size());
	green.reserve(pts.size());
	blue.reserve(pts.size());
	for (std::size_t i = 0; i < pts.size(); ++i)
	{
		Label_handle label = labels[static_cast<std::size_t>(label_indices[i])];
		unsigned r = 0, g = 0, b = 0;
		if (label == ground)
		{
			r = 245;
			g = 180;
			b = 0;
		}
		else if (label == vegetation)
		{
			r = 0;
			g = 255;
			b = 27;
		}
		else if (label == roof)
		{
			r = 255;
			g = 0;
			b = 170;
		}
		red.push_back(r);
		green.push_back(g);
		blue.push_back(b);
	}
	std::ofstream f(fileName, std::ios::binary);
	CGAL::IO::set_binary_mode(f);

	CGAL::IO::write_LAS_with_properties(f, CGAL::make_range(boost::counting_iterator<std::size_t>(0), boost::counting_iterator<std::size_t>(pts.size())),
		CGAL::make_las_point_writer(CGAL::make_property_map(pts)),
		std::make_pair(CGAL::make_property_map(red), CGAL::LAS_property<unsigned char>("red")),
		std::make_pair(CGAL::make_property_map(green), CGAL::LAS_property<unsigned char>("green")),
		std::make_pair(CGAL::make_property_map(blue), CGAL::LAS_property<unsigned char>("blue")));
	
	f.close();
}


int main(int argc, char** argv)
{

	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " input_file" << std::endl;
		return EXIT_FAILURE;
	}

	const std::string filename = argv[1];
	std::cerr << "Reading input" << std::endl;
	std::vector<Point> pts;

	// Load file

	if (!CGAL::IO::read_points(filename, std::back_inserter(pts), CGAL::parameters::point_map(CGAL::Identity_property_map<Point>())))
	{
		std::cerr << "Error: cannot read file " << filename << std::endl;
		return EXIT_FAILURE;
	}
	/*
	if (!(CGAL::IO::read_points(filename, std::back_inserter(pts),
								// the PLY reader expects a binary file by default
								CGAL::parameters::use_binary_mode(true))))
	{
		std::cerr << "Error: cannot read " << filename << std::endl;
		return EXIT_FAILURE;
	}
	*/
	float grid_resolution = 0.34f;
	unsigned int number_of_neighbors = 6;
	std::cerr << "Computing useful structures" << std::endl;
	Iso_cuboid_3 bbox = CGAL::bounding_box(pts.begin(), pts.end());
	Planimetric_grid grid(pts, Pmap(), bbox, grid_resolution);
	Neighborhood neighborhood(pts, Pmap());
	Local_eigen_analysis eigen = Local_eigen_analysis::create_from_point_set(pts, Pmap(), neighborhood.k_neighbor_query(number_of_neighbors));
	float radius_neighbors = 1.7f;
	float radius_dtm = 15.0f;
	std::cerr << "Computing features" << std::endl;
	Feature_set features;
	features.begin_parallel_additions(); // No effect in sequential mode
	Feature_handle distance_to_plane = features.add<Distance_to_plane>(pts, Pmap(), eigen);
	Feature_handle dispersion = features.add<Dispersion>(pts, Pmap(), grid, radius_neighbors);
	Feature_handle elevation = features.add<Elevation>(pts, Pmap(), grid, radius_dtm);
	features.end_parallel_additions(); // No effect in sequential mode
	Label_set labels;
	// Init name only
	Label_handle ground = labels.add("ground");
	// Init name and color
	Label_handle vegetation = labels.add("vegetation", CGAL::IO::Color(0, 255, 0));
	// Init name, Color and standard index (here, ASPRS building index)
	Label_handle roof = labels.add("roof", CGAL::IO::Color(255, 0, 0), 6);
	std::cerr << "Setting weights" << std::endl;
	Classifier classifier(labels, features);
	classifier.set_weight(distance_to_plane, 6.75e-2f);
	classifier.set_weight(dispersion, 5.45e-1f);
	classifier.set_weight(elevation, 1.47e1f);
	std::cerr << "Setting effects" << std::endl;
	classifier.set_effect(ground, distance_to_plane, Classifier::NEUTRAL);
	classifier.set_effect(ground, dispersion, Classifier::NEUTRAL);
	classifier.set_effect(ground, elevation, Classifier::PENALIZING);
	classifier.set_effect(vegetation, distance_to_plane, Classifier::FAVORING);
	classifier.set_effect(vegetation, dispersion, Classifier::FAVORING);
	classifier.set_effect(vegetation, elevation, Classifier::NEUTRAL);
	classifier.set_effect(roof, distance_to_plane, Classifier::NEUTRAL);
	classifier.set_effect(roof, dispersion, Classifier::NEUTRAL);
	classifier.set_effect(roof, elevation, Classifier::FAVORING);
	// Run classification
	std::cerr << "Classifying" << std::endl;
	std::vector<int> label_indices(pts.size(), -1);
	CGAL::Real_timer t;
	t.start();
	Classification::classify<CGAL::Parallel_if_available_tag>(pts, labels, classifier, label_indices);
	t.stop();
	std::cerr << "Raw classification performed in " << t.time() << " second(s)" << std::endl;
	t.reset();

	//std::string output_filename = "classification_raw.ply";
	//write_ply(pts, output_filename, ground, vegetation, roof, label_indices, labels);
	std::string output_filename = "classification_raw.laz";
	write_las(pts, output_filename, ground, vegetation, roof, label_indices, labels);

	t.start();
	Classification::classify_with_local_smoothing<CGAL::Parallel_if_available_tag>(pts, Pmap(), labels, classifier,
		neighborhood.sphere_neighbor_query(radius_neighbors),
		label_indices);
	t.stop();
	std::cerr << "Classification with local smoothing performed in " << t.time() << " second(s)" << std::endl;
	t.reset();
	/*t.start();
	Classification::classify_with_graphcut<CGAL::Parallel_if_available_tag>(pts, Pmap(), labels, classifier,
																			neighborhood.k_neighbor_query(12),
																			0.2f, 4, label_indices);
	t.stop();
	std::cerr << "Classification with graphcut performed in " << t.time() << " second(s)" << std::endl;*/
	// Save the output in a colored PLY format

	//output_filename = "classification_smoothed.ply";
	output_filename = "classification_smoothed.laz";
	write_las(pts, output_filename, ground, vegetation, roof, label_indices, labels);

	//write_ply(pts, output_filename, ground, vegetation, roof, label_indices, labels);

	std::cerr << "All done" << std::endl;
	return EXIT_SUCCESS;
}
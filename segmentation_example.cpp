#if defined (_MSC_VER) && !defined (_WIN64)
#pragma warning(disable:4244) // boost::number_distance::distance()
// converts 64 to 32 bits integers
#endif
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Classification.h>
#include <CGAL/bounding_box.h>
#include <CGAL/tags.h>
#include <CGAL/IO/read_points.h>
#include <CGAL/IO/write_ply_points.h>
#include <CGAL/Real_timer.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
typedef CGAL::Parallel_if_available_tag Concurrency_tag;
typedef CGAL::Simple_cartesian<double> Kernel;
typedef Kernel::Point_3 Point;
typedef Kernel::Iso_cuboid_3 Iso_cuboid_3;
typedef std::vector<Point> Point_range;
typedef CGAL::Identity_property_map<Point> Pmap;
namespace Classification = CGAL::Classification;
typedef Classification::Sum_of_weighted_features_classifier Classifier;
typedef Classification::Planimetric_grid<Kernel, Point_range, Pmap>             Planimetric_grid;
typedef Classification::Point_set_neighborhood<Kernel, Point_range, Pmap>       Neighborhood;
typedef Classification::Local_eigen_analysis                                    Local_eigen_analysis;
typedef Classification::Label_handle                                            Label_handle;
typedef Classification::Feature_handle                                          Feature_handle;
typedef Classification::Label_set                                               Label_set;
typedef Classification::Feature_set                                             Feature_set;
typedef Classification::Feature::Distance_to_plane<Point_range, Pmap>           Distance_to_plane;
typedef Classification::Feature::Elevation<Kernel, Point_range, Pmap>           Elevation;
typedef Classification::Feature::Vertical_dispersion<Kernel, Point_range, Pmap> Dispersion;
typedef Classification::Feature::Color_channel<Kernel, Point_range, Pmap>       Color_channel;

//may return 0 when not able to detect
const auto processor_count = std::thread::hardware_concurrency();


int main(int argc, char** argv)
{
	const std::string filename = (argc > 1) ? argv[1] : CGAL::data_file_path("meshes/b9.ply");

	CGAL::Real_timer t;
	std::cerr << "Reading input" << std::endl;
	std::vector<Point> pts;
	t.start();
	if (!(CGAL::IO::read_points(filename, std::back_inserter(pts),
		// the PLY reader expects a binary file by default
		CGAL::parameters::use_binary_mode(true))))
	{
		std::cerr << "Error: cannot read " << filename << std::endl;
		return EXIT_FAILURE;
	}

	std::cerr << "Done in " << t.time() << " sec" << std::endl;

	float grid_resolution = 0.34f;
	unsigned int number_of_neighbors = 6;
	std::cerr << "Computing useful structures" << std::endl;
	t.stop();
	t.reset();
	t.start();
	Iso_cuboid_3 bbox = CGAL::bounding_box(pts.begin(), pts.end());
	Planimetric_grid grid(pts, Pmap(), bbox, grid_resolution);
	Neighborhood neighborhood(pts, Pmap());
	Local_eigen_analysis eigen = Local_eigen_analysis::create_from_point_set(pts, Pmap(), neighborhood.k_neighbor_query(number_of_neighbors));
	float radius_neighbors = 1.7f;
	float radius_dtm = 15.0f;

	std::cerr << "Done in " << t.time() << " sec" << std::endl;

	std::cerr << "Computing features" << std::endl;

	t.reset();
	t.start();
	Feature_set features;
	features.begin_parallel_additions(); // No effect in sequential mode
	Feature_handle distance_to_plane = features.add<Distance_to_plane>(pts, Pmap(), eigen);
	Feature_handle dispersion = features.add<Dispersion>(pts, Pmap(), grid,radius_neighbors);
	Feature_handle elevation = features.add<Elevation>(pts, Pmap(), grid,	radius_dtm);
	Feature_handle green = features.add<Color_channel>(pts, Pmap(), grid, radius_neighbors, 1);
	
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

	std::cerr << "Done in " << t.time() << " sec" << std::endl;
	t.reset();
	t.start();

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

	std::cerr << "Done in " << t.time() << " sec" << std::endl;
	t.reset();
	t.start();
	std::vector<int> label_indices(pts.size(), -1);
	// Run classification
	/*std::cerr << "Classifying" << std::endl;
	t.start();
	
	

	Classification::classify<CGAL::Parallel_if_available_tag>(pts, labels, classifier, label_indices);
	t.stop();
	std::cerr << "Raw classification performed in " << t.time() << " second(s)" << std::endl;
	t.reset();*/

	
	Classification::classify_with_local_smoothing<CGAL::Parallel_if_available_tag>
		(pts, Pmap(), labels, classifier,
			neighborhood.sphere_neighbor_query(radius_neighbors),
			label_indices);
	
	std::cerr << "Classification with local smoothing performed in " << t.time() << " second(s)" << std::endl;
	t.stop();
	//t.start();


	/*

	Classification::classify_with_graphcut<CGAL::Parallel_if_available_tag>
		(pts, Pmap(), labels, classifier,
			neighborhood.k_neighbor_query(12),
			0.2f, processor_count, label_indices);
	//t.stop();
	std::cerr << "Classification with graphcut performed in " << t.time() << " second(s)" << std::endl;
	// Save the output in a colored PLY format
	//t.start();
	*/
	t.reset();
	t.start();
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
			r = 245; g = 180; b = 0;
		}
		else if (label == vegetation)
		{
			r = 0; g = 255; b = 27;
		}
		else if (label == roof)
		{
			r = 255; g = 0; b = 170;
		}
		red.push_back(r);
		green.push_back(g);
		blue.push_back(b);
	}
	std::ofstream f("classification.ply", std::ios::binary);

	// Set binary ply
	CGAL::IO::set_binary_mode(f); // The PLY file will be written in the binary format
		
	CGAL::IO::write_PLY_with_properties
	(f, CGAL::make_range(boost::counting_iterator<std::size_t>(0),
		boost::counting_iterator<std::size_t>(pts.size())),
		CGAL::make_ply_point_writer(CGAL::make_property_map(pts)),
		std::make_pair(CGAL::make_property_map(red), CGAL::PLY_property<unsigned char>("red")),
		std::make_pair(CGAL::make_property_map(green), CGAL::PLY_property<unsigned char>("green")),
		std::make_pair(CGAL::make_property_map(blue), CGAL::PLY_property<unsigned char>("blue")));
	std::cerr << "Writing done in " << t.time() << " sec" << std::endl;
	std::cerr << "All done" << std::endl;
	return EXIT_SUCCESS;
}
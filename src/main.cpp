#include "PathTracer.hpp"
#include "sfml_interface/interface.h"

#include "utils/logger.h"

// settings ===================================================================

const int VERBOSITY = 2; // 2 for detailed log of some things

const int WINDOW_WIDTH  = 1000;
const int WINDOW_HEIGHT = 1000;

const int PIXEL_SCREEN_WIDTH  = 200;
// const int PIXEL_SCREEN_HEIGHT = 1000;

const int 	 PIXEL_SAMPLING   = 1;
const int 	 MAX_TRACE_DEPTH  = 10;

const double GAMMA_CORRECTION = 0.36;

const int DEFAULT_THREADS_CNT = 4;

// ============================================================================

const FramePostproc ACCUMULATOR_FRAME_POSTPROC = FramePostproc::copy;
const FramePostproc RENDERED_FRAME_POSTPROC    = FramePostproc::copy;

const double ACCUMULATOR_FRAME_POSTPROC_RADIUS = 1;
const double RENDERED_FRAME_POSTPROC_RADIUS = 1;

// ============================================================================

const int REAL_SCREEN_WIDTH   = 100;
const int REAL_SCREEN_HEIGHT  = 100;

const Vec3d  CAMERA_POS 	  = {-100, 50, 50};
const double CAMERA_DIST 	  = 100;

const double RESOLUTION_COEF  =  (double) PIXEL_SCREEN_WIDTH / (double)  REAL_SCREEN_WIDTH;

const Vec3d  BACKGROUND_COLOR = {0, 0, 0};

// ============================================================================

HittableList scene_gen(int sphere_cnt = 1, Vec3d delta = VEC3D_ZERO);

HittableList *cornell_box_objects();
Scene 		 *cornell_box_scene();

// ============================================================================

int main(int argc, char* argv[]) {
	Brans::srand_sse(time(NULL));
	srand(time(NULL));

	logger.page_cut();

	conf_Render conf_render(REAL_SCREEN_WIDTH * RESOLUTION_COEF, REAL_SCREEN_HEIGHT * RESOLUTION_COEF,
							MAX_TRACE_DEPTH,
							PIXEL_SAMPLING,
							GAMMA_CORRECTION,
							BACKGROUND_COLOR);

    conf_Verbosity  conf_verbos(VERBOSITY);
    conf_SystemInfo conf_sysinf(vec3d_randlong() % 10000, DEFAULT_THREADS_CNT, nullptr);
    conf_PathTracer conf_pt(conf_render, conf_verbos, conf_sysinf);

	conf_pt.update_from_command_line_args(argc, argv);

	logger.info("Zepher", "process (%d) started", conf_pt.sysinf.timestamp);
    logger.info("Zepher", "using (%d) threads", conf_pt.sysinf.kernel_cnt);

    Scene *scene = cornell_box_scene();
	logger.info("Zepher", "scene prepared");

	SFML_Interface *interface = new SFML_Interface("Zepher", scene, conf_pt, WINDOW_WIDTH, WINDOW_HEIGHT, PIXEL_SAMPLING);

	logger.info("sfml_interface", "launching, press F to make a screenshot");

	logger.page_cut("interface.run()");

	// scene->objects->dump_bvh(0);

	interface->set_postprocs(ACCUMULATOR_FRAME_POSTPROC,
							 RENDERED_FRAME_POSTPROC,
							 ACCUMULATOR_FRAME_POSTPROC_RADIUS,
							 RENDERED_FRAME_POSTPROC_RADIUS);

	auto interface_interactor = new std::thread(&SFML_Interface::run, interface);

	// interface->interaction_loop();

	interface->stop();

	// interface_interactor->join();

	logger.page_cut();

	logger.info("Zepher", "process [%d] finished", conf_pt.sysinf.timestamp);

	return 0;
}

//=============================================================================

Scene *cornell_box_scene() {
	Camera *camera = new Camera(CAMERA_POS, {1, 0, 0}, 
    							CAMERA_DIST,
    							REAL_SCREEN_WIDTH, REAL_SCREEN_HEIGHT,
    							RESOLUTION_COEF);
	HittableList *objects = cornell_box_objects();

    auto tree = objects->get_bvh_tree();
	
	Scene *scene = new Scene(camera, tree);
	return scene;
}

HittableList *cornell_box_objects() {
	HittableList *scene = new HittableList;

	Material *m_white = new m_Lambertian({255, 255, 255});
	Material *m_red   = new m_Lambertian({255,   0,   0});
	Material *m_green = new m_Lambertian({  0, 255,   0});

    Material *m_mirror = new m_Metal({255, 255, 125}, 0.05);
    Material *m_glass  = new m_Dielectric({255, 255, 255}, 2);
    Material *m_glass2  = new m_Dielectric({130, 130, 255}, 2.4);

	// Material *m_box_1 = new m_Lambertian({255, 255, 255}); ^^^^^^^^^^^^^
	// Material *m_box_2 = new m_Lambertian({255, 255, 255});

	Light *l_rect_light = new l_Diffuse({255, 255, 255});

	m_Lambertian *m_rect_light = new m_Lambertian(Vec3d(255, 255, 255));
	m_rect_light->set_emitter(l_rect_light);
	m_rect_light->to_affect_emitter = 0;

	const double heigh = 100;
	const double width = 100;
	const double depth = 100;

	const double light_size_coef = 0.32;
	const double box_coef = 0.1;

	const double l_h = heigh - VEC3_EPS;
	const double l_w = width * light_size_coef;
	const double l_d = depth * light_size_coef;

	Hittable *rect_ceil  = new h_RectXY({    0,     0, heigh}, {depth, width, heigh}, m_white);
	Hittable *rect_floor = new h_RectXY({    0,     0,     0}, {depth, width, heigh}, m_white);
	Hittable *rect_fwall = new h_RectYZ({depth,     0,     0}, {depth, width, heigh}, m_white);
	Hittable *rect_rwall = new h_RectXZ({    0,     0,    -1}, {depth,     0, heigh + 1}, m_red  );
	Hittable *rect_lwall = new h_RectXZ({    -100, width,     0}, {depth, width, heigh}, m_green);
 	
	Hittable *rect_light = new h_RectXY({depth / 2 - l_d / 2, width / 2 - l_w / 2, l_h}, {depth / 2 + l_d / 2, width / 2 + l_w / 2, l_h}, m_rect_light);
	Hittable *rect_light_floor = new h_RectXY({depth / 2 - l_d / 2, width / 2 - l_w / 2, -10}, {depth / 2 + l_d / 2, width / 2 + l_w / 2, l_h}, m_rect_light);

	Hittable *box_1 = new h_Box({ depth * box_coef,  width * box_coef, 0},
								{-depth * box_coef, -width * box_coef, heigh * 0.75},
								m_mirror);

	Hittable *box_2 = new h_Box({ depth * box_coef,  width * box_coef, 0},
								{-depth * box_coef, -width * box_coef, heigh * 0.25},
                                m_glass);

    Hittable *box_3 = new h_Box({ depth * 3 * box_coef,  width * 2.5 *  box_coef, 0},
                                {-depth * 3 *  box_coef, -width *  2.5 * box_coef, heigh * 0.03},
                                m_glass2);

	Hittable *rot_box_1 = new inst_Translate(new inst_RotZ(box_1,  Pi/3.5), {60, 70, 0});
	Hittable *rot_box_2 = new inst_Translate(new inst_RotZ(box_2, -Pi/3), {30, 25, 0});
    Hittable *rot_box_3 = new inst_Translate(box_3, {depth / 2 - l_d / 2 + 6, width / 2 - l_w / 2 + 16.5, l_h - 22.5});


	// Material *mat = new m_Lambertian({10, 145, 200});
	// Hittable *box = new h_Box({ 0,  0, 0},
	// 						  {7.5, 7.5, 150},
	// 						   mat);
	// box = new inst_RotX(new inst_RotZ(box, Pi/8), Pi/4);
	// box = new inst_Translate(box, {70, 0, 0});

	// scene->insert(box);

	// cenzure for the girl pic
	// Material *m_cenzure = new m_Dielectric({255, 255, 255}, 1, -1, 0.2);
	// Hittable *rect_cenze1 = new h_RectYZ({50, 44, 58}, {50, 56, 64}, m_cenzure);
	// scene->insert(rect_cenze1);

	scene->insert(rect_ceil );
	scene->insert(rect_floor);
	scene->insert(rect_fwall);
	scene->insert(rect_lwall);
	scene->insert(rect_rwall);

	scene->insert(rect_light);


	Material *m_model = new m_Metal({255, 255, 30}, 0.05);	

	Hittable *model = new Model("../models/whip.obj", {m_glass}, {0, 0, 0}, 17,false); // remove ../ if you build tracer NOT in build dir
	model = new inst_RotZ(new inst_RotX(model, -Pi/2), Pi/2);
	model = new inst_Translate(model, {10, 50, 0});
	
	// scene->insert(model);

	// Hittable *sp = new h_Sphere({95, 25, 75}, 15, m_model);
	// scene->insert(sp);
	// sp = new h_Sphere({95, 75, 25}, 15, m_model);
	// scene->insert(sp);

	Material *mt = new m_Dielectric({255, 0.874 * 255, 0.768 * 255}, 2);
	Hittable *sp = new h_RectYZ({0, 30, 30}, {0, 70, 70}, mt);
	scene->insert(new inst_Translate(new inst_RotY(sp, -Pi/3), {30, 0, 0}));

	// Material *mat = new m_Lambertian(new t_Checkered(Color{125, 175, 225}, VEC3D_ZERO, VEC3D_ONE * 0.25));
	// Light *lig = new l_Diffuse({255, 255, 255});
	// mat->set_emitter(lig);
	// Hittable *sphere = new h_Sphere({depth / 2, width / 3, heigh / 3}, heigh / 4, mat);
	// scene->insert(sphere);

	if (0) { // so not to see "unused" warnings
		scene->insert(rot_box_1);
		scene->insert(rot_box_2);
		scene->insert(rot_box_3);
		scene->insert(rect_light_floor);
	}

	return scene;
}

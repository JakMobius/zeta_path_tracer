#include <utils/fast_random.hpp>
#include "collection_materials.h"

m_Lambertian::m_Lambertian(const Color &albedo_):
Material(),
albedo(new t_SolidColor(albedo_)),
to_affect_emitter(1)
{}

m_Lambertian::m_Lambertian(Texture *texture_):
Material(),
albedo(texture_),
to_affect_emitter(1)
{}

bool m_Lambertian::scatter(const Ray &, const HitRecord &hitrec, Color &attenuation, Ray &scattered) const {
	Vec3d scatter_direction = hitrec.n;
    scatter_direction += Vec3d::random_unit(); // TODO plz
	if (scatter_direction.is_zero()) {
		scatter_direction = hitrec.n;
	}

    scattered = Ray(hitrec.p, scatter_direction);
    attenuation = albedo->value(hitrec.surf_x, hitrec.surf_y, hitrec.p);
    return true;
}

void m_Lambertian::affect_emitter(Vec3d &emitted, const double sx, const double sy, const Vec3d &point) const {
	emitted *= albedo->value(sx, sy, point) / d_MAXRGB;
}

//=============================================================================

m_Metal::m_Metal(const Color &albedo_, const double fuzziness_):
Material(),
albedo(new t_SolidColor(albedo_)),
fuzziness(fuzziness_),
to_affect_emitter(1)
{}

m_Metal::m_Metal(Texture *texture_, const double fuzziness_):
Material(),
albedo(texture_),
fuzziness(fuzziness_),
to_affect_emitter(1)
{}

bool m_Metal::scatter(const Ray &ray, const HitRecord &hitrec, Color &attenuation, Ray &scattered) const {
    Vec3d scatter_direction = Vec3d::random_in_unit_sphere();
    scatter_direction *= fuzziness;
    scatter_direction += reflect(ray.dir, hitrec.n);

    scattered = Ray(hitrec.p, scatter_direction);
    attenuation = albedo->value(hitrec.surf_x, hitrec.surf_y, hitrec.p);
    return true;
}

void m_Metal::affect_emitter(Vec3d &, const double, const double, const Vec3d&) const {}

//=============================================================================

m_Dielectric::m_Dielectric(const Color &albedo_, const double refrac_coef_, const double roughness_):
Material(),
albedo(new t_SolidColor(albedo_)),
refrac_coef(refrac_coef_),
roughness(roughness_),
to_affect_emitter(1)
{}

m_Dielectric::m_Dielectric(Texture *texture_,    const double refrac_coef_, const double roughness_):
Material(),
albedo(texture_),
refrac_coef(refrac_coef_),
roughness(roughness_),
to_affect_emitter(1)
{}

bool m_Dielectric::scatter(const Ray &ray, const HitRecord &hitrec, Color &attenuation, Ray &scattered) const {
	attenuation = albedo->value(hitrec.surf_x, hitrec.surf_y, hitrec.p);
	double rc = hitrec.front_hit ? (1 / refrac_coef) : refrac_coef;

	double cos_theta = fmin(-(ray.dir.dot(hitrec.n)) / ray.dir.len(), 1.0);
	double sin_theta = sqrt(1.0 - cos_theta * cos_theta);

	bool cannot_refract = rc * sin_theta > 1.0;
	Vec3d scatter_direction;

    unsigned int random_values[4];
    Brans::rand_sse(random_values);
    double first_random_value = (double) random_values[0] / (double) UINT32_MAX;
    double second_random_value = (double) random_values[1] / (double) UINT32_MAX;

	if (cannot_refract || reflectance(cos_theta, rc) > first_random_value || (roughness > 0 && second_random_value < roughness)) {
		scatter_direction = reflect(ray.dir, hitrec.n);
	} else {
		scatter_direction = refract(ray.dir, hitrec.n, rc);
	}

	scattered = Ray(hitrec.p, scatter_direction);

    return true;
}

double m_Dielectric::reflectance(double cosine, double ref_idx) { // Schlick Approximation from RayTracingInOneWeekend
    double r0 = (1 - ref_idx) / (1 + ref_idx);
    r0 = r0 * r0;
    return r0 + (1 - r0)*pow((1 - cosine), 5);
}

void m_Dielectric::affect_emitter(Vec3d &, const double, const double, const Vec3d&) const {}

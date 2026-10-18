#include "core/collision_gjk.h"

namespace era_engine
{
    GJKInternalSuccess update_gjk_simplex(GJKSimplex& s, const GJKSupportPoint& a, vec3& dir)
    {
        if (s.num_points == 2)
        {
            // Triangle case
            vec3 ao = -a.minkowski;
            vec3 ab = s.b.minkowski - a.minkowski;
            vec3 ac = s.c.minkowski - a.minkowski;
            vec3 abc = cross(ab, ac);

            vec3 abp = cross(ab, abc);
            if (dot(ao, abp) > 0.f)
            {
                s.c = a;
                dir = cross_aba(ab, ao);
                return GJK_DONT_STOP;
            }
            vec3 acp = cross(abc, ac);
            if (dot(ao, acp) > 0.f)
            {
                s.b = a;
                dir = cross_aba(ac, ao);
                return GJK_DONT_STOP;
            }

            // Sort so that normal abc of triangle points outside (negative new search dir)
            if (dot(ao, abc) >= 0.f)
            {
                s.d = s.b;
                s.b = a;
                s.num_points = 3;
                dir = abc;
                return GJK_DONT_STOP;
            }
            if (dot(ao, -abc) >= 0.f)
            {
                s.d = s.c;
                s.c = s.b;
                s.b = a;
                s.num_points = 3;
                dir = -abc;
                return GJK_DONT_STOP;
            }

            std::cerr << "GJK ERROR 0\n";
            return GJK_UNEXPECTED_ERROR;
        }
        if (s.num_points == 3)
        {
            // Tetrahedron case
            vec3 ao = -a.minkowski;
            vec3 ab = s.b.minkowski - a.minkowski;
            vec3 ac = s.c.minkowski - a.minkowski;
            vec3 ad = s.d.minkowski - a.minkowski;

            vec3 bcd = cross(s.c.minkowski - s.b.minkowski, s.d.minkowski - s.b.minkowski);
            if (dot(bcd, dir) > 0.00001f || dot(bcd, s.b.minkowski) < -0.00001f)
            {
                std::cerr << "GJK ERROR 0\n";
                return GJK_UNEXPECTED_ERROR;
            }

            // Normals of faces (point outside)
            vec3 abc = cross(ac, ab);
            vec3 abd = cross(ab, ad);
            vec3 adc = cross(ad, ac);

            int32 flags = 0;
            const int32 over_abc_flag = 1;
            const int32 over_abd_flag = 2;
            const int32 over_adc_flag = 4;

            flags |= (dot(abc, ao) > 0.f) ? over_abc_flag : 0;
            flags |= (dot(abd, ao) > 0.f) ? over_abd_flag : 0;
            flags |= (dot(adc, ao) > 0.f) ? over_adc_flag : 0;

            if (flags == (over_abc_flag | over_abd_flag | over_adc_flag))
            {
                std::cerr << "GJK ERROR 0\n";
                return GJK_UNEXPECTED_ERROR;
            }

            if (flags == 0)
            {
                return GJK_STOP;
            }

            if (flags == over_abc_flag)
            {
            overABC1:
                if (dot(cross(abc, ab), ao) > 0.f)
                {
                    // New line: ab
                    s.c = a;
                    s.num_points = 2;
                    dir = cross_aba(ab, ao);
                    return GJK_DONT_STOP;
                }
            overABC2:
                if (dot(cross(ac, abc), ao) > 0.f)
                {
                    // New line: ac
                    s.b = a;
                    s.num_points = 2;
                    dir = cross_aba(ac, ao);
                    return GJK_DONT_STOP;
                }

                // Stay in triangle case: bca
                s.d = a;
                dir = abc;
                return GJK_DONT_STOP;
            }

            if (flags == over_abd_flag)
            {
            overABD1:
                if (dot(cross(abd, ad), ao) > 0.f)
                {
                    // New line: ad
                    s.b = s.d;
                    s.c = a;
                    s.num_points = 2;
                    dir = cross_aba(ad, ao);
                    return GJK_DONT_STOP;
                }
            overABD2:
                if (dot(cross(ab, abd), ao) > 0.f)
                {
                    // New line: ab
                    s.c = a;
                    s.num_points = 2;
                    dir = cross_aba(ab, ao);
                    return GJK_DONT_STOP;
                }

                // Stay in triangle case: bad
                s.c = a;
                dir = abd;
                return GJK_DONT_STOP;
            }

            if (flags == over_adc_flag)
            {
            overADC1:
                if (dot(cross(adc, ac), ao) > 0.f)
                {
                    // New line: ac
                    s.b = a;
                    s.num_points = 2;
                    dir = cross_aba(ac, ao);
                    return GJK_DONT_STOP;
                }
            overADC2:
                if (dot(cross(ad, adc), ao) > 0.f)
                {
                    // New line: ad
                    s.b = a;
                    s.c = s.d;
                    s.num_points = 2;
                    dir = cross_aba(ad, ao);
                    return GJK_DONT_STOP;
                }

                // Stay in triangle case: acd
                s.b = a;
                dir = adc;
                return GJK_DONT_STOP;
            }

            if (flags == (over_abc_flag | over_abd_flag))
            {
                if (dot(cross(abc, ab), ao) > 0.f)
                {
                    goto overABD1;
                }
                goto overABC2;
            }

            if (flags == (over_abd_flag | over_adc_flag))
            {
                if (dot(cross(abd, ad), ao) > 0.f)
                {
                    goto overADC1;
                }
                goto overABD2;
            }

            if (flags == (over_adc_flag | over_abc_flag))
            {
                if (dot(cross(adc, ac), ao) > 0.f)
                {
                    goto overABC1;
                }
                goto overADC2;
            }

            std::cerr << "GJK ERROR 1\n";
            return GJK_UNEXPECTED_ERROR;
        }

        std::cerr << "GJK ERROR 2\n";
        return GJK_UNEXPECTED_ERROR;
    }
}
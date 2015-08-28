#include <math.h>
#include "bfe_helper.h"

void c_density(double *xyz, int K,
               double M, double r_s,
               double *Snlm, double *Tnlm,
               int nmax, int lmax, double *dens) {

    int i,j,k, n,l,m;
    double r, s, X, phi;
    for (k=0; k<K; k++) {
        j = 3*k;
        r = sqrt(xyz[j]*xyz[j] + xyz[j+1]*xyz[j+1] + xyz[j+2]*xyz[j+2]);
        s = r/r_s;
        X = xyz[j+2]/r; // cos(theta)
        phi = atan2(xyz[j+1], xyz[j+0]);
        for (n=0; n<(nmax+1); n++) {
            for (l=0; l<(lmax+1); l++) {
                for (m=0; m<(lmax+1); m++) {
                    if (m > l)
                        continue;

                    i = m + (lmax+1) * (l + (lmax+1) * n);
                    dens[k] += rho_nlm(s, phi, X, n, l, m) * (Snlm[i]*cos(m*phi) +
                                                              Tnlm[i]*sin(m*phi));
                }
            }
        }
        dens[k] *= M / (r_s*r_s*r_s);
    }
}

void c_potential(double *xyz, int K,
                 double G, double M, double r_s,
                 double *Snlm, double *Tnlm,
                 int nmax, int lmax, double *val) {

    int i,j,k, n,l,m;
    double r, s, X, phi;
    for (k=0; k<K; k++) {
        j = 3*k;
        r = sqrt(xyz[j]*xyz[j] + xyz[j+1]*xyz[j+1] + xyz[j+2]*xyz[j+2]);
        s = r/r_s;
        X = xyz[j+2]/r; // cos(theta)
        phi = atan2(xyz[j+1], xyz[j+0]);

        i = 0;
        for (n=0; n<(nmax+1); n++) {
            for (l=0; l<(lmax+1); l++) {
                for (m=0; m<(lmax+1); m++) {
                    if (m > l) {
                        i++;
                        continue;
                    }

                    val[k] += phi_nlm(s, phi, X, n, l, m) * (Snlm[i]*cos(m*phi) +
                                                             Tnlm[i]*sin(m*phi));
                    i++;
                }
            }
        }
        val[k] *= G*M/r_s;
    }
}

void c_gradient(double *xyz, int K,
                double G, double M, double r_s,
                double *Snlm, double *Tnlm,
                int nmax, int lmax, double *grad) {

    int i,j,k, n,l,m;
    double r, s, X, phi;
    double sintheta, cosphi, sinphi, tmp;
    double tmp_grad[3] = {0.};

    for (k=0; k<K; k++) {
        j = 3*k;
        r = sqrt(xyz[j]*xyz[j] + xyz[j+1]*xyz[j+1] + xyz[j+2]*xyz[j+2]);
        s = r/r_s;
        X = xyz[j+2]/r; // cos(theta)
        phi = atan2(xyz[j+1], xyz[j+0]);

        sintheta = sqrt(1 - X*X);
        cosphi = cos(phi);
        sinphi = sin(phi);

        i = 0;
        for (n=0; n<(nmax+1); n++) {
            for (l=0; l<(lmax+1); l++) {
                for (m=0; m<(lmax+1); m++) {
                    if (m > l) {
                        i++;
                        continue;
                    }

                    tmp = (Snlm[i]*cos(m*phi) + Tnlm[i]*sin(m*phi));

                    sph_grad_phi_nlm(s, phi, X, n, l, m, &tmp_grad[0]);
                    tmp_grad[0] *= tmp; // r
                    tmp_grad[1] *= tmp * sintheta / s; // theta
                    tmp_grad[2] *= (Tnlm[i]*cos(m*phi) - Snlm[i]*sin(m*phi)) / (s*sintheta); // phi

                    grad[j+0] += sintheta*cosphi*tmp_grad[0] + X*cosphi*tmp_grad[1] - sinphi*tmp_grad[2];
                    grad[j+1] += sintheta*sinphi*tmp_grad[0] + X*sinphi*tmp_grad[1] + cosphi*tmp_grad[2];
                    grad[j+2] += X*tmp_grad[0] - sintheta*tmp_grad[1];

                    i++;
                }
            }
        }
        grad[j+0] *= -G*M/(r_s*r_s);
        grad[j+1] *= -G*M/(r_s*r_s);
        grad[j+2] *= -G*M/(r_s*r_s);
    }
}

double scf_value(double t, double *pars, double *q) {
    /*  pars:
        - G (Gravitational constant)
        - m (mass scale)
        - r_s (length scale)
        - nmax
        - lmax
        [- sin_coeff, cos_coeff]
    */
    double G = pars[0];
    double M = pars[1];
    double r_s = pars[2];
    int nmax = (int)pars[3];
    int lmax = (int)pars[4];

    double val[1] = {0.};
    double _val;
    int n,l,m;

    int num_coeff = 0;
    for (n=0; n<(nmax+1); n++) {
        for (l=0; l<(lmax+1); l++) {
            for (m=0; m<(lmax+1); m++) {
                num_coeff++;
            }
        }
    }

    c_potential(&q[0], 1,
                G, M, r_s,
                &pars[5], &pars[5+num_coeff],
                nmax, lmax, &val[0]);

    _val = val[0];
    return _val;
}

void scf_gradient(double t, double *pars, double *q, double *grad) {
    /*  pars:
        - G (Gravitational constant)
        - m (mass scale)
        - r_s (length scale)
        - nmax
        - lmax
        [- sin_coeff, cos_coeff]
    */
    double G = pars[0];
    double M = pars[1];
    double r_s = pars[2];
    int nmax = (int)pars[3];
    int lmax = (int)pars[4];

    int n,l,m;

    int num_coeff = 0;
    for (n=0; n<(nmax+1); n++) {
        for (l=0; l<(lmax+1); l++) {
            for (m=0; m<(lmax+1); m++) {
                num_coeff++;
            }
        }
    }

    c_gradient(&q[0], 1,
               G, M, r_s,
               &pars[5], &pars[5+num_coeff],
               nmax, lmax, &grad[0]);
}
/*        Generated by TAPENADE     (INRIA, Tropics team)
    Tapenade 3.10 (r5498) - 20 Jan 2015 09:48
*/
#include <math.h>
#include <stdlib.h>

//#include "gmm.h"
// This throws error on n<1
void arr_max_b(int n, double *x, double *xb, double arr_maxb) {
    double m = x[0];
    double mb;
    int i;
    int branch;
    double arr_max;
    for (i = 1; i < n; ++i)
        if (x[i] > m) {
            m = x[i];
            pushcontrol1b(1);
        } else
            pushcontrol1b(0);
    mb = arr_maxb;
    for (i = n-1; i > 0; --i) {
        popcontrol1b(&branch);
        if (branch != 0) {
            xb[i] = xb[i] + mb;
            mb = 0.0;
        }
    }
    xb[0] = xb[0] + mb;
}

void logsumexp_b(int n, double *x, double *xb, double logsumexpb) {
    double mx;
    double mxb;
    double logsumexp;
    double tempb;
    mx = arr_max(n, x);
    double semx = 0.;
    double semxb;
    int i;
    for (i = 0; i < n; ++i)
        semx = semx + exp(x[i] - mx);
    semxb = logsumexpb/semx;
    mxb = logsumexpb;
    for (i = n-1; i > -1; --i) {
        tempb = exp(x[i]-mx)*semxb;
        xb[i] = xb[i] + tempb;
        mxb = mxb - tempb;
    }
    arr_max_b(n, x, xb, mxb);
}

// d dim
// k number of gaussians
// n number of points
// alphas k logs of mixture weights (unnormalized), so
//			weights = exp(log_alphas) / sum(exp(log_alphas))
// means d*k component means
// inv_cov_factors (d*(d+1)/2)*k parametrizing lower triangular 
//					square roots of inverse covariances log of diagonal 
//					is first d params
// x d*n points
// err 1 output
// To generate params in MATLAB given covariance C :
//           L = inv(chol(C, 'lower'));
//           inv_cov_factor = [log(diag(L)); L(au_tril_indices(d, -1))]
void gmm_b(int d, int k, int n, double *alphas, double *alphasb, double *means
        , double *meansb, double *inv_cov_factors, double *inv_cov_factorsb, 
        double *x, double *err, double *errb) {
    const double PI;
    const double CONSTANT;
    int ik, ix, id, i, j, icf_off, Lparamsidx;
    int icf_sz = d*(d+1)/2;
    double *lseb = (double *)malloc(k*sizeof(double));
    double *lse = (double *)malloc(k*sizeof(double));
    double *Ldiagb = (double *)malloc(d*sizeof(double));
    double *Ldiag = (double *)malloc(d*sizeof(double));
    double *xcenteredb = (double *)malloc(d*sizeof(double));
    double *xcentered = (double *)malloc(d*sizeof(double));
    double *mahalb = (double *)malloc(d*sizeof(double));
    double *mahal = (double *)malloc(d*sizeof(double));
    double sumlog_Ldiag, sqsum_mahal, slse, lse_alphas;
    double sumlog_Ldiagb, sqsum_mahalb, slseb, lse_alphasb;
    double result1;
    double result1b;
	int adFrom;
	memset(alphasb, 0, k*sizeof(double));
	memset(meansb, 0, d*k*sizeof(double));
	memset(inv_cov_factorsb, 0, icf_sz*k*sizeof(double));
    for (ix = 0; ix < n; ++ix)
        for (ik = 0; ik < k; ++ik) {
            icf_off = ik*icf_sz;
            sumlog_Ldiag = 0.;
            for (id = 0; id < d; ++id) {
                sumlog_Ldiag = sumlog_Ldiag + inv_cov_factors[icf_off + id];
                pushreal8(Ldiag[id]);
                Ldiag[id] = exp(inv_cov_factors[icf_off + id]);
            }
            for (id = 0; id < d; ++id) {
                pushreal8(xcentered[id]);
                xcentered[id] = x[ix*d + id] - means[ik*d + id];
                pushreal8(mahal[id]);
                mahal[id] = Ldiag[id]*xcentered[id];
            }
            Lparamsidx = d;
            for (i = 0; i < d; ++i) {
                adFrom = i + 1;
                for (j = adFrom; j < d; ++j) {
                    pushreal8(mahal[j]);
                    mahal[j] = mahal[j] + inv_cov_factors[icf_off+Lparamsidx]*
                        xcentered[i];
                    pushinteger4(Lparamsidx);
                    Lparamsidx = Lparamsidx + 1;
                }
                pushinteger4(adFrom);
            }
            sqsum_mahal = 0.;
            for (id = 0; id < d; ++id)
                sqsum_mahal = sqsum_mahal + mahal[id]*mahal[id];
            pushreal8(lse[ik]);
            lse[ik] = alphas[ik] + sumlog_Ldiag - 0.5*sqsum_mahal;
        }
    slseb = *errb;
    lse_alphasb = -(n*(*errb));
    logsumexp_b(k, alphas, alphasb, lse_alphasb);
	memset(mahalb, 0, d*sizeof(double));
	memset(xcenteredb, 0, d*sizeof(double));
	memset(Ldiagb, 0, d*sizeof(double));
	memset(lseb, 0, k*sizeof(double));
    for (ix = n-1; ix > -1; --ix) {
        result1b = slseb;
        logsumexp_b(k, lse, lseb, result1b);
        for (ik = k-1; ik > -1; --ik) {
            popreal8(&lse[ik]);
            alphasb[ik] = alphasb[ik] + lseb[ik];
            sumlog_Ldiagb = lseb[ik];
            sqsum_mahalb = -(0.5*lseb[ik]);
            lseb[ik] = 0.0;
            for (id = d-1; id > -1; --id)
                mahalb[id] = mahalb[id] + 2*mahal[id]*sqsum_mahalb;
            icf_off = ik*icf_sz;
            for (i = d-1; i > -1; --i) {
                popinteger4(&adFrom);
                for (j = d-1; j > adFrom-1; --j) {
                    popinteger4(&Lparamsidx);
                    popreal8(&mahal[j]);
                    inv_cov_factorsb[icf_off + Lparamsidx] = inv_cov_factorsb[
                        icf_off + Lparamsidx] + xcentered[i]*mahalb[j];
                    xcenteredb[i] = xcenteredb[i] + inv_cov_factors[icf_off+
                        Lparamsidx]*mahalb[j];
                }
            }
            for (id = d-1; id > -1; --id) {
                popreal8(&mahal[id]);
                Ldiagb[id] = Ldiagb[id] + xcentered[id]*mahalb[id];
                xcenteredb[id] = xcenteredb[id] + Ldiag[id]*mahalb[id];
                mahalb[id] = 0.0;
                popreal8(&xcentered[id]);
                meansb[ik*d + id] = meansb[ik*d + id] - xcenteredb[id];
                xcenteredb[id] = 0.0;
            }
            for (id = d-1; id > -1; --id) {
                popreal8(&Ldiag[id]);
                inv_cov_factorsb[icf_off + id] = inv_cov_factorsb[icf_off + id
                    ] + exp(inv_cov_factors[icf_off+id])*Ldiagb[id];
                Ldiagb[id] = 0.0;
                inv_cov_factorsb[icf_off + id] = inv_cov_factorsb[icf_off + id
                    ] + sumlog_Ldiagb;
            }
        }
    }
    free(mahal);
    free(mahalb);
    free(xcentered);
    free(xcenteredb);
    free(Ldiag);
    free(Ldiagb);
    free(lse);
    free(lseb);
    *errb = 0.0;
}
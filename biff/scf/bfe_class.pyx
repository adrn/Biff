# coding: utf-8
# cython: boundscheck=False
# cython: nonecheck=False
# cython: cdivision=True
# cython: wraparound=False

from __future__ import division, print_function

# Standard library
from collections import OrderedDict
from libc.math cimport M_PI

# Third party
from astropy.constants import G
import numpy as np
cimport numpy as np
np.import_array()
import cython
cimport cython

# Gala
from gala.units import galactic
from gala.potential.potential.cpotential cimport CPotentialWrapper
from gala.potential.potential.cpotential import CPotentialBase

cdef extern from "src/funcdefs.h":
    ctypedef double (*densityfunc)(double t, double *pars, double *q, int n_dim) nogil
    ctypedef double (*energyfunc)(double t, double *pars, double *q, int n_dim) nogil
    ctypedef void (*gradientfunc)(double t, double *pars, double *q, int n_dim, double *grad) nogil

cdef extern from "potential/src/cpotential.h":
    enum:
        MAX_N_COMPONENTS = 16

    ctypedef struct CPotential:
        int n_components
        int n_dim
        densityfunc density[MAX_N_COMPONENTS]
        energyfunc value[MAX_N_COMPONENTS]
        gradientfunc gradient[MAX_N_COMPONENTS]
        int n_params[MAX_N_COMPONENTS]
        double *parameters[MAX_N_COMPONENTS]

cdef extern from "bfe.h":
    double scf_value(double t, double *pars, double *q, int n_dim) nogil
    double scf_density(double t, double *pars, double *q, int n_dim) nogil
    void scf_gradient(double t, double *pars, double *q, int n_dim, double *grad) nogil

__all__ = ['SCFPotential']

cdef class SCFWrapper(CPotentialWrapper):

    def __init__(self, G, parameters, q0):
        self.init([G] + list(parameters), np.ascontiguousarray(q0))
        self.cpotential.value[0] = <energyfunc>(scf_value)
        self.cpotential.density[0] = <densityfunc>(scf_density)
        self.cpotential.gradient[0] = <gradientfunc>(scf_gradient)

class SCFPotential(CPotentialBase):
    r"""
    SCFPotential(m, r_s, Snlm, Tnlm, units=None)

    An SCF / basis function expansion potential. Follows the
    convention used in Hernquist & Ostriker (1992) and
    Lowing et al. (2011) for representing all coefficients as
    real quantities.

    Parameters
    ----------
    m : numeric
        Scale mass.
    r_s : numeric
        Scale length.
    Snlm : array_like
        Array of coefficients for the cosine terms of the expansion.
        This should be a 3D array with shape `(nmax+1, lmax+1, lmax+1)`,
        where `nmax` is the number of radial expansion terms and `lmax`
        is the number of spherical harmonic `l` terms.
    Tnlm : array_like
        Array of coefficients for the sine terms of the expansion.
        This should be a 3D array with shape `(nmax+1, lmax+1, lmax+1)`,
        where `nmax` is the number of radial expansion terms and `lmax`
        is the number of spherical harmonic `l` terms.
    units : iterable
        Unique list of non-reducable units that specify (at minimum) the
        length, mass, time, and angle units.

    """
    def __init__(self, m, r_s, Snlm, Tnlm, units=None):
        Snlm = np.array(Snlm)
        Tnlm = np.array(Tnlm)

        if Snlm.shape != Tnlm.shape:
            raise ValueError("Shape of coefficient arrays must match! "
                             "({} vs {})".format(Snlm.shape, Tnlm.shape))

        # extra parameters
        nmax = Tnlm.shape[0]-1
        lmax = Tnlm.shape[1]-1

        parameters = OrderedDict()
        ptypes = OrderedDict()

        parameters['m'] = m
        ptypes['m'] = 'mass'

        parameters['r_s'] = r_s
        ptypes['r_s'] = 'length'

        parameters['nmax'] = nmax
        parameters['lmax'] = lmax
        parameters['Snlm'] = Snlm
        parameters['Tnlm'] = Tnlm

        # TODO: I need to save the full 3D Snlm, Tnlm and ravel elsewhere

        super(SCFPotential, self).__init__(parameters=parameters,
                                           parameter_physical_types=ptypes,
                                           units=units,
                                           Wrapper=SCFWrapper,
                                           c_only=['nmax', 'lmax']) # don't expose these in the .parameters dictionary

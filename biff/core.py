# coding: utf-8

from __future__ import division, print_function

__author__ = "adrn <adrn@astro.columbia.edu>"

# Third-party
import numpy as np
import scipy.integrate as si

# Project
from ._computecoeff import Snlm_integrand, Tnlm_integrand, STnlm_discrete

__all__ = ['compute_coeffs', 'compute_coeffs_discrete']

def compute_coeffs(density_func, nlm, M, r_s, args=(), **kwargs):
    """
    Compute the expansion coefficients for representing the input
    density function as a basis function expansion.

    This is largely based on the formalism outlined in
    Hernquist & Ostriker (1992). The radial basis functions
    are taken to be Hernquist functions.

    Computing the coefficients involves computing triple integrals
    which are computationally expensive. For an example of how to
    parallelize the computation of the coefficients, see
    ``examples/parallel_compute_Anlm.py``.

    Parameters
    ----------
    density_func : function, callable
        A function or callable object to evaluate the density at a
        given position. The call format must be of the form:
        ``density_func(x, y, z, M, r_s, args)`` where ``x,y,z`` are
        cartesian coordinates, ``M`` is a scale mass, ``r_s`` a scale
        radius, and ``args`` is an iterable containing any other
        arguments needed by the density function.
    nlm : iterable
        A list or iterable of integers containing the values of n, l,
        and m, e.g., ``nlm = [n,l,m]``.
    M : numeric
        Scale mass.
    r_s : numeric
        Scale radius.
    args : iterable (optional)
        A list or iterable of any other arguments needed by the density
        function.

    kwargs
        Any additional keyword arguments are passed through to
        `~scipy.integrate.nquad` as options, `opts`.

    Returns
    -------
    Snlm : float
        The value of the cosine expansion coefficient.
    Snlm_err : float
        An estimate of the uncertainty in the coefficient value.
    Tnlm : float
        The value of the sine expansion coefficient.
    Tnlm_err : float
        An estimate of the uncertainty in the coefficient value.

    """
    nlm = np.array(nlm).astype(np.int32)

    kwargs.setdefault('limit', 256)
    kwargs.setdefault('epsrel', 1E-10)

    limits = [[0,2*np.pi], # phi
              [-1,1.], # X (cos(theta))
              [-1,1.]] # xsi

    Snlm = si.nquad(Snlm_integrand,
                    ranges=limits,
                    args=(density_func, nlm[0], nlm[1], nlm[2], M, r_s, args),
                    opts=kwargs)

    Tnlm = si.nquad(Tnlm_integrand,
                    ranges=limits,
                    args=(density_func, nlm[0], nlm[1], nlm[2], M, r_s, args),
                    opts=kwargs)

    return Snlm, Tnlm

def compute_coeffs_discrete(xyz, mass, nlm, r_s):
    """
    Compute the expansion coefficients for representing the input
    density function as a basis function expansion.

    This is largely based on the formalism outlined in
    Hernquist & Ostriker (1992). The radial basis functions
    are taken to be Hernquist functions.

    Computing the coefficients involves computing triple integrals
    which are computationally expensive. For an example of how to
    parallelize the computation of the coefficients, see
    ``examples/parallel_compute_Anlm.py``.

    Parameters
    ----------
    xyz : array_like
    mass : array_like
        Mass of each discrete object.
    nlm : iterable
        A list or iterable of integers containing the values of n, l,
        and m, e.g., ``nlm = [n,l,m]``.
    r_s : numeric
        Scale radius.

    Returns
    -------
    Snlm : float
        The value of the cosine expansion coefficient.
    Tnlm : float
        The value of the sine expansion coefficient.

    """
    xyz = np.ascontiguousarray(np.atleast_2d(xyz))
    mass = np.ascontiguousarray(np.atleast_1d(mass))

    nlm = np.array(nlm).astype(np.int32)
    # _args = np.array(args)arrayxyz =

    r = np.sqrt(np.sum(xyz**2, axis=-1))
    s = r / r_s
    phi = np.arctan2(xyz[:,1], xyz[:,0])
    X = xyz[:,2] / r

    Snlm, Tnlm = STnlm_discrete(s, phi, X, mass, nlm[0], nlm[1], nlm[2])

    return Snlm, Tnlm

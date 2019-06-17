#!/usr/bin/env python3
import os
import sys


import binascii
import struct

try:
    from Crypto import Random
    from Crypto.IO import PKCS8, PEM
    from Crypto.Util.py3compat import tobytes, bord, tostr
    from Crypto.Util.asn1 import DerSequence

    from Crypto.Math.Numbers import Integer
    from Crypto.Math.Primality import (test_probable_prime,
                                       generate_probable_prime, COMPOSITE)

    from Crypto.PublicKey import (_expand_subject_public_key_info,
                                  _create_subject_public_key_info,
                                  _extract_subject_public_key_info)


    from Crypto.PublicKey import RSA
except:
    from Cryptodome import Random
    from Cryptodome.IO import PKCS8, PEM
    from Cryptodome.Util.py3compat import tobytes, bord, tostr
    from Cryptodome.Util.asn1 import DerSequence
    from Cryptodome.Math.Numbers import Integer
    from Cryptodome.Math.Primality import (test_probable_prime,
                                       generate_probable_prime, COMPOSITE)
    from Cryptodome.PublicKey import (_expand_subject_public_key_info,
                                  _create_subject_public_key_info,
                                  _extract_subject_public_key_info)
    from Cryptodome.PublicKey import RSA
                             
                              
def generate_probable_prime(**kwargs):
    """Generate a random probable prime.
    The prime will not have any specific properties
    (e.g. it will not be a *strong* prime).
    Random numbers are evaluated for primality until one
    passes all tests, consisting of a certain number of
    Miller-Rabin tests with random bases followed by
    a single Lucas test.
    The number of Miller-Rabin iterations is chosen such that
    the probability that the output number is a non-prime is
    less than 1E-30 (roughly 2^{-100}).
    This approach is compliant to `FIPS PUB 186-4`__.
    :Keywords:
      exact_bits : integer
        The desired size in bits of the probable prime.
        It must be at least 160.
      randfunc : callable
        An RNG function where candidate primes are taken from.
      prime_filter : callable
        A function that takes an Integer as parameter and returns
        True if the number can be passed to further primality tests,
        False if it should be immediately discarded.
    :Return:
        A probable prime in the range 2^exact_bits > p > 2^(exact_bits-1).
    .. __: http://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.186-4.pdf
    """

    exact_bits = kwargs.pop("exact_bits", None)
    randfunc = kwargs.pop("randfunc", None)
    prime_filter = kwargs.pop("prime_filter", lambda x: True)
    if kwargs:
        raise ValueError("Unknown parameters: " + kwargs.keys())

    if exact_bits is None:
        raise ValueError("Missing exact_bits parameter")
    #if exact_bits < 160:
    #    raise ValueError("Prime number is not big enough.")

    if randfunc is None:
        randfunc = Random.new().read

    result = COMPOSITE
    while result == COMPOSITE:
        candidate = Integer.random(exact_bits=exact_bits,
                                   randfunc=randfunc) | 1
        if not prime_filter(candidate):
            continue
        result = test_probable_prime(candidate, randfunc)
    return candidate

def generate(bits, randfunc=None, e=65537):
    """allow to generate small keys for test purposes
    """

    if e % 2 == 0 or e < 3:
        raise ValueError("RSA public exponent must be a positive, odd integer larger than 2.")

    if randfunc is None:
        randfunc = Random.get_random_bytes

    d = n = Integer(1)
    e = Integer(e)

    while n.size_in_bits() != bits and d < (1 << (bits // 2)):
        # Generate the prime factors of n: p and q.
        # By construciton, their product is always
        # 2^{bits-1} < p*q < 2^bits.
        size_q = bits // 2
        size_p = bits - size_q

        min_p = min_q = (Integer(1) << (2 * size_q - 1)).sqrt()
        if size_q != size_p:
            min_p = (Integer(1) << (2 * size_p - 1)).sqrt()

        def filter_p(candidate):
            return candidate > min_p and (candidate - 1).gcd(e) == 1

        p = generate_probable_prime(exact_bits=size_p,
                                    randfunc=randfunc,
                                    prime_filter=filter_p)

        min_distance = Integer(1) << (max(bits // 2 - 100,10))

        def filter_q(candidate):
            return (candidate > min_q and
                    (candidate - 1).gcd(e) == 1 and
                    abs(candidate - p) > min_distance)

        q = generate_probable_prime(exact_bits=size_q,
                                    randfunc=randfunc,
                                    prime_filter=filter_q)

        n = p * q
        lcm = (p - 1).lcm(q - 1)
        d = e.inverse(lcm)

    if p > q:
        p, q = q, p

    u = p.inverse(q)

    return RSA.RsaKey(n=n, e=e, d=d, p=p, q=q, u=u)

if (len(sys.argv) > 3) | (len(sys.argv) < 0) :
    print("ERROR: incorrect arguments")
    print("Usage:")
    print("genrsakey.py [bitlen] [e]")
    exit()

bitlen=2048
e=65537

if len(sys.argv)>1:
    bitlen=int(sys.argv[1],0)


if len(sys.argv)>2:
    e=int(sys.argv[2],0)

if bitlen<1024:
    #small key, use our unsafe custom code
    key = generate(bitlen, None, e)
else:
    #real key, use Python lib with all the checks that comes with it
    key = RSA.generate(bitlen, None, e)

print("d=%d"%key.d)
print("e=%d"%key.e)
print("n=%d"%key.n)

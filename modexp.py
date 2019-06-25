#!/usr/bin/env python3

def _extended_euclidean(a, b):
  """Helper function that runs the extended Euclidean algorithm, which
     finds x and y st a * x + b * y = gcd(a, b). Used for gcd and modular
     division. Returns x, y, gcd(a, b)"""
  flip = b > a
  if flip:
    a, b = b, a
  if b == 0:
    return 1, 0, a
  x, y, gcd = _extended_euclidean(b, a % b)
  ny = y
  nx = x - (a // b) * y
  assert(a * ny + b * nx == gcd)
  if flip:
    ny, nx = nx, ny
  return ny, nx, gcd

def gcd(a, b):
  """Returns the greatest common denominator of a, b."""
  assert(a >= 0 and b >= 0)
  _, _, d = _extended_euclidean(a, b)
  return d

def modinv(x, n):
  """Returns the inverse of x mod n if it exists, or None if not."""
  a, b, gcd = _extended_euclidean(x, n)
  if gcd != 1:
    return None
  else:
    return a % n

p = 17
q = 23
e = 5
print("e=",e)
n = p * q
print("n=",n)
phi_n = (p-1)*(q-1)

public_key = (e, n)
d = modinv(e, phi_n)
print("d=",d)
assert(d is not None)

def modexp3(b,m):
    """e=3, use addition chain"""
    b2=(b*b)%m
    b3=(b*b2)%m
    assert(b3==pow(b,3,m))
    return b3

def modexp5(b,m):
    """e=5, use addition chain"""
    b2=(b*b)%m
    b4=(b2*b2)%m
    b5=(b*b4)%m
    assert(b5==pow(b,5,m))
    return b5

for i in range(0,n):
    modexp3(i,n)
    modexp5(i,n)


def m_mult(x,y,m,mp,n):
    b=2
    A=0
    y0=y&1
    for i in range(0,n):
        a0=A&1
        xi=(x>>i)&1
        ui = (a0 ^ (xi & y0)) & mp
        if xi:
            A+=y
        if ui:
            A+=m
        A=A>>1
    if A>=m:
        A-=m
    return A

def m_mult_test(x,y,m):
    b=2
    n=m.bit_length()
    R=1<<n
    mp=1
    xyRinv=m_mult(x,y,m,mp,n)
    s=(xyRinv*R)%m
    assert(s==((x*y)%m))
    return s

for i in range(0,n):
    for j in range(0,n):
        m_mult_test(i,j,n)

def m_modexp(x,e,m,R,R2,mp,n,ma=0,mb=0):
    assert(x<m)
    #assert(x>=1)
    if x==0:
        return -1
    xp = m_mult(x,R2,m,mp,n)

    A = R
    for i in range(e.bit_length()-1,-1,-1):
        #ma,mb injection can be done here or before the loop. For security it is better here
        xp=(xp+ma)%m
        xp=(xp+mb)%m

        A=m_mult(A,A,m,mp,n)
        ei=(e>>i)&1
        if ei:
            A=m_mult(A,xp,m,mp,n)
    A=m_mult(A,1,m,mp,n)
    A=(A+ma)%m
    A=(A+mb)%m
    return A

def m_modexp_test(x,y,m):
    b=2
    n=m.bit_length()
    R=1<<n
    mp=1
    R2=(R*R)%m
    R=R%m
    s=m_modexp(x,y,m,R,R2,mp,n)
    assert(s==pow(x,y,m))
    return s

for i in range(1,n):
    for j in range(0,n):
        m_modexp_test(i,j,n)


forged_n=n-1
forged_d = modinv(e,forged_n-1)
print("forged_n=",forged_n)
print("forged_d=",forged_d)

def tryit(modexp_func,dump=False):
  undetected=0;
  for i in range(1,n):
    sig=pow(i,d,n)
    if sig!=0:
        c=modexp_func(sig,e,n)
        if c!=i:
            print(i,d,n,"->",sig,e,"->",c)
        assert(c==i)
    forged_sig=pow(i,forged_d,forged_n)
    detected = modexp_func(forged_sig,e,forged_n)!=i
    if not detected:
      undetected+=1
    if dump:
      print("i=",i,", forged_sig=",forged_sig,end="")
      if detected:
        print("-> detected")
      else:
        print()
  print(modexp_func,undetected)
  return undetected

def modexp_basic(x,y,m):
    return pow(x,y,m)

mhcp_n=n.bit_length()
mhcp_R=(1<<(mhcp_n))
mhcp_mp=1
mhcp_R2=(mhcp_R*mhcp_R)%n
mhcp_R=mhcp_R%n
mhcp_ma=13
mhcp_mb=n-mhcp_ma
def modexp_mhcp(x,y,m):
    s=m_modexp(x,y,m,mhcp_R,mhcp_R2,mhcp_mp,mhcp_n)
    return s

def modexp_mhcp2(x,y,m):
    s=m_modexp(x,y,m,mhcp_R,mhcp_R2,mhcp_mp,mhcp_n,mhcp_ma,mhcp_mb)
    return s

tryit(modexp_basic)
tryit(modexp_mhcp)
tryit(modexp_mhcp2)

#!/usr/bin/env python3

class Montgommery(object):
    """Montgommery modular exponentiation model"""

    def __init__(self, modulus):
        self.b=2
        self.m = modulus
        self.n=self.m.bit_length()
        self.R=1<<self.n
        self.mp=1
        self.R2=(self.R*self.R)%self.m
        self.R=self.R%self.m

    @staticmethod
    def mult(x,y,m,mp=1):
        n=m.bit_length()
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

    @staticmethod
    def s_modexp(x,e,m,R,R2,mp=1,ma=0,mb=0):
        assert(x<m)
        #assert(x>=1)
        if x==0:
            return -1
        xp = Montgommery.mult(x,R2,m,mp)
        A = R
        for i in range(e.bit_length()-1,-1,-1):
            #ma,mb injection can be done here or before the loop. For security it is better here
            xp=(xp+ma)%m
            xp=(xp+mb)%m

            A=Montgommery.mult(A,A,m,mp)
            ei=(e>>i)&1
            if ei:
                A=Montgommery.mult(A,xp,m,mp)
        A=Montgommery.mult(A,1,m,mp)
        A=(A+ma)%m
        A=(A+mb)%m
        return A

    def modexp(self,x,y):
        return self.s_modexp(x,y,self.m,self.R,self.R2,self.mp,0,0)

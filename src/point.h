#ifndef _POINT_INC
#define _POINT_INC

/*
Written by DSC aprox. 1996
1.7.98 Translation into good-looking C++
*/


class point
        {
        public:
                double x,y,z;    /* oops... this makes rendering faster! */

                point();
                point(double,double,double);
                //point(text &);
                point(const point &);

                void create(double,double,double);
                //void load(text &);

                point operator+(const point &);       // addition
                point operator-(const point &);       // substraction
                int operator==(const point&);        // comparison
                int operator!=(const point &);        // negated comparison
                point& operator=(const point&);        // copy
                point operator*(double);      // scaling
				point operator/(double);	  // inv. scaling
                double operator*(point);      // dot product
                point operator^(point);       // cross product

                point interpolate(point,double,point,double);
                double modulo();
                double modulosq();
				double distance(point);
				double distanceman(point);
				double distancemanxz(point);

                void normalize();
                void negate();
                int into(point, point);
				float distancePointLine(point,point);//distance from point this to line ab

   				void rotatex(double);
				void rotatey(double);
				void rotatey(float,float); //rotatey passing sin(ang) and cos(ang) values
				void rotatez(double);

				bool infrustum(point,double,double);	// viewer,yaw of viewer,aperture,caller is point to test

				~point();
        };

#endif






/**
 * @brief TESTING ONLY
 * Its just a vector. Simple test data structure
 * 
 */

namespace math {
    struct vector{
        double x,y,z;
        vector();
        vector(double x);
        vector(double x, double y);
        vector(double x, double y, double z);

        vector operator+ (const vector& r);
        vector operator* (const double& s);
    };
}

math::vector::vector (){
    x = y = z = 0;
}
math::vector::vector (double _x){
    x = _x;
    y = z = 0;
}
math::vector::vector (double _x, double _y){
    x = _x;
    y = _y;
    z = 0;
}
math::vector::vector (double _x, double _y, double _z){
    x = _x;
    y = _y;
    z = _z;
}

math::vector math::vector::operator+ (const math::vector& r){
    return math::vector(x+r.x,y+r.y,z+r.z);
}

math::vector math::vector::operator*(const double& s){
   return math::vector(x*s, y*s, z*s);
}
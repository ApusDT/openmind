//
// Created by Сергей Кривонос on 01.09.17.
//
#include "Fraction.h"
#include "Integer.h"
#include "Sum.h"
#include "Product.h"
#include <boost/lexical_cast.hpp>

namespace std{
    template<>
    struct is_signed<boost::multiprecision::cpp_int> {
        bool value = true;
    };
}

namespace omnn{
namespace math {
	Fraction::Fraction(const Integer& n)
		: base(n, 1)
	{
	}
    
    Fraction::Fraction(const boost::multiprecision::cpp_dec_float_100& f)
    {
        auto s = boost::lexical_cast<std::string>(f);
        if (s=="nan") {
            throw s;
        }
        auto doti = s.find_first_of('.');
        if (doti == -1)
        {
            numerator() = Integer(boost::multiprecision::cpp_int(f));
            denominator() = 1;
        }
        else
        {
            auto afterDotI = doti + 1;
            auto fsz = s.length() - afterDotI;
            denominator() = 10_v^Integer(fsz);
            numerator() = Integer(boost::multiprecision::cpp_int(s.c_str()+afterDotI));
            if (s[0]=='-') {
                numerator() = -numerator();
            }
            s.erase(doti);
            numerator() += Integer(boost::multiprecision::cpp_int(s)) * denominator();
        }
    }
    
	Valuable Fraction::operator -() const
    {
        return Fraction(-getNumerator(), getDenominator());
    }

    void Fraction::optimize()
    {
//        if (!optimizations) {
//            hash = numerator().Hash() ^ denominator().Hash();
//            return;
//        }
    reoptimize_the_fraction:
        numerator().optimize();
        denominator().optimize();
        
        while(denominator().IsFraction())
        {
            auto dn = cast(denominator());
            numerator() *= dn->denominator();
            denominator() = std::move(dn->numerator());
        }

//        if (numerator().IsSum()) {
//            Become(numerator()/denominator());
//            return;
//        }
        
        if (denominator().IsInt())
        {
            if (denominator() == 1_v)
            {
                Become(std::move(numerator()));
                return;
            }
            else if (denominator() < 0) {
                numerator() = -numerator();
                denominator() = -denominator();
            }
        }
        
        if (numerator().IsExponentiation()) {
            auto e = Exponentiation::cast(numerator());
            auto& exp = e->getExponentiation();
            if (exp.IsInt() && exp < 0) {
                denominator() *= e->getBase() ^ (-exp);
                numerator() = 1;
            } else if (exp.IsFraction()) {
                auto f = Fraction::cast(exp);
                auto in = e->getBase() / (denominator() ^ f->Reciprocal());
                if (in.IsInt())
                {
                    Become(in ^ exp);
                    return;
                }
            }
        }

        // integers
        if (numerator().IsInt())
        {
            auto n = Integer::cast(numerator());
            auto dn = Integer::cast(denominator());
            if (*n == 0_v)
            {
                if (dn && *dn == 0_v)
                    throw "NaN";
                Become(std::move(numerator()));
                return;
            }
            if (dn)
            {
                if (*n < 0 && *dn < 0) {
                    numerator() = -numerator();
                    denominator() = -denominator();
                    goto reoptimize_the_fraction;
                }
                if (*dn == 1_v)
                {
                    Become(std::move(*n));
                    return;
                }
                if (*dn == -1_v)
                {
                    Become(-*n);
                    return;
                }
                auto d = boost::gcd(n->ca(), dn->ca());
                if (d != 1) {
                    numerator() /= Integer(d);
                    denominator() /= Integer(d);
                    optimize();
                }
            }
        }
        else
        {
            std::vector<Variable> coVa;

            if (numerator().IsProduct()) {
                //if (denominator().IsProduct()) {
                    Become(numerator() / denominator());
                    return;
                //}
                //else
                //{
                //    auto n = Product::cast(numerator());
                    //if (n->Has(denominator()))
					//{
     //                   Become(*n / denominator());
     //                   return;
     //               }
     //           }
            }
            else if (denominator().IsProduct())
            {
                auto dn = Product::cast(denominator());
                if (dn->Has(numerator())) {
                    denominator() /= numerator();
                    numerator() = 1_v;
                    goto reoptimize_the_fraction;
                }
                else if (numerator().IsInt() || numerator().IsSimpleFraction()) {
                    for (auto& m : *dn)
                    {
                        if (m.IsVa()) {
                            numerator() *= m ^ -1;
                        }
                        else if (m.IsExponentiation()) {
                            auto e = Exponentiation::cast(m);
                            numerator() *= e->getBase() ^ -e->getExponentiation();
                        }
                        else
                            numerator() /= m;
                    }
                    Become(std::move(numerator()));
                    return;
                }
            }
            else if (denominator().FindVa() && !denominator().IsSum())
            {
                Become(Product{numerator(),Exponentiation(denominator(), -1)});
            }
            else // no products
            {
                // sum
//                auto s = Sum::cast(numerator());
//                if (s) {
//                    auto sum(std::move(*s));
//                    sum /= denominator();
//                    Become(std::move(sum));
//                    return;
//                }
            }
        }
        
        if (IsFraction()) {
            hash = numerator().Hash() ^ denominator().Hash();
        }
    }
    
    Valuable& Fraction::operator +=(const Valuable& v)
    {
        auto i = cast(v);
        if (i){
			auto f = *i;
            if(denominator() != f.denominator()) {
                f.numerator() *= denominator();
                f.denominator() *= denominator();
                numerator() *= i->denominator();
                denominator() *= i->denominator();
            }

            numerator() += f.numerator();
            optimize();
        }
        else
        {
            if(v.IsInt() && IsSimple())
            {
                auto i = Integer::cast(v);
                *this += Fraction(*i);
            }
            else
            {
                return Become(Sum {*this, v});
            }
        }
        return *this;
    }

    Valuable& Fraction::operator *=(const Valuable& v)
    {
        if (v.IsFraction())
        {
            auto f = cast(v);
            numerator() *= f->numerator();
            denominator() *= f->denominator();
        }
        else if (v.IsInt())
        {
            numerator() *= v;
        }
        else
        {
            return Become(v * *this);
        }
		
        optimize();
        return *this;
    }

    Valuable& Fraction::operator /=(const Valuable& v)
    {
        if (v.IsFraction())
		{
            auto f = cast(v);
            numerator() *= f->denominator();
			denominator() *= f->numerator();
		}
        else if (v.IsProduct())
        {
            for(auto& _ : *Product::cast(v))
                *this /= _;
        }
        else
        {
			denominator() *= v;
        }
		optimize();
		return *this;
    }

    Valuable& Fraction::operator %=(const Valuable& v)
    {
        Integer d(*this / v);
		return *this -= d * v;
    }

    Valuable& Fraction::operator^=(const Valuable& v)
    {
        if(v.IsInt())
        {
            auto i = v.ca();
            if (i != 0_v) {
                if (i > 1_v) {
                    auto a = *this;
                    for (auto n = i; n > 1_v; --n) {
                        *this *= a;
                    }
                    optimize();
                    return *this;
                }
            }
            else { // zero
                if (numerator() == 0_v)
                    throw "NaN"; // feel free to handle this properly
                else
                    return Become(1_v);
            }
        }
        else if(IsSimple())
        {
            auto f = Fraction::cast(v);
            if (f->IsSimple())
            {
                auto n = f->numerator();
                auto dn = f->denominator();

                if (n != 1_v)
                    *this ^= n;
                Valuable nroot;
                bool rootFound = false;
                Valuable left =0, right = *this;

                while (!rootFound)
                {
                    nroot = left +(right - left) / 2_v;
                    auto result = nroot ^ dn;
                    if (result == *this)
                    {
                        rootFound = true;
                        return Become(std::move(nroot));
                    }
                    else if (*this < result)
                    {
                        right = nroot;
                    }
                    else
                    {
                        left = nroot;
                    }
                }
            }
        }

        return Become(Exponentiation(*this, v));
    }
    

    bool Fraction::operator <(const Valuable& v) const
    {
        if (v.IsFraction())
        {
            auto f = cast(v);
            return numerator() * f->denominator() < f->numerator() * denominator();
        }
        else if (v.IsInt())
        {
            return numerator() < v * denominator();
        }
        else
            return base::operator <(v);
    }
    
    Fraction::operator double() const
    {
        return static_cast<double>(numerator()) / static_cast<double>(denominator());
    }
    
    Valuable& Fraction::sq(){
        numerator().sq();
        denominator().sq();
        optimize();
        return *this;
    }

    Valuable Fraction::sqrt() const
    {
        return numerator().sqrt() / denominator().sqrt();
    }

    std::ostream& Fraction::print_sign(std::ostream& out) const
    {
        return out << '/';
    }
    
    const Valuable::vars_cont_t& Fraction::getCommonVars() const
    {
        vars = numerator().getCommonVars();
        for(auto& r : denominator().getCommonVars())
        {
            vars[r.first] -= r.second;
        }
        return vars;
    }

    bool Fraction::IsComesBefore(const Valuable& v) const
    {
//        auto va1 = FindVa();
//        auto va2 = v.FindVa();
//        return !va1 && !va2
//            ? (IsSimple() && (v.IsInt() || (v.IsFraction() && Fraction::cast(v)->IsSimple())) ? operator<(v) : str().length() < v.str().length())
//                : (va1 && va2
//                   ? str().length() < v.str().length()
//                   : va1!=nullptr );
        auto mve = base::getMaxVaExp();
        auto vmve = v.getMaxVaExp();
        auto is = mve > vmve;
        if (mve != vmve)
        {}
        else if (v.IsFraction())
        {
            if (IsSimple() && v.IsSimpleFraction())
                is = *this < v;
            else
            {
                auto f = cast(v);
                is = numerator().IsComesBefore(f->numerator()) || denominator().IsComesBefore(f->denominator());
            }
//            auto e = cast(v);
//            bool numerator()IsVa = numerator().IsVa();
//            bool vbaseIsVa = e->numerator().IsVa();
//            if (numerator()IsVa && vbaseIsVa)
//                is = denominator() == e->denominator() ? numerator().IsComesBefore(e->numerator()) : denominator() > e->denominator();
//            else if(numerator()IsVa)
//                is = false;
//            else if(vbaseIsVa)
//                is = true;
//            else if(numerator() == e->numerator())
//                is = denominator().IsComesBefore(e->denominator());
//            else if(denominator() == e->denominator())
//                is = numerator().IsComesBefore(e->numerator());
//            else
//            {
//                auto expComesBefore = denominator().IsComesBefore(e->denominator());
//                auto ebaseComesBefore = numerator().IsComesBefore(e->numerator());
//                is = ebaseComesBefore || expComesBefore;//expComesBefore==ebaseComesBefore || str().length() > e->str().length();
//            }
        }
        else if(v.IsProduct())
            is = Product{*this}.IsComesBefore(v);
        else if(v.IsSum())
            is = Sum{*this}.IsComesBefore(v);
        else if(v.IsVa())
            is = FindVa();
        else if(v.IsInt())
            is = true;
        else
            IMPLEMENT

        return is;
    }
    
    Fraction::operator unsigned char() const
    {
        return static_cast<unsigned char>(static_cast<Integer>(*this));
    }
  
    Fraction::operator a_int() const
    {
        if (!IsSimple()) {
            IMPLEMENT
        }
        return static_cast<a_int>(numerator())/static_cast<a_int>(denominator());
    }
    
    Fraction::operator boost::multiprecision::cpp_dec_float_100() const
    {
        if (IsSimple())
        {
            auto& num = *Integer::cast(numerator());
            boost::multiprecision::cpp_dec_float_100 f(num.as_const_base_int_ref());
            auto & d = *Integer::cast(denominator());
            f /= boost::multiprecision::cpp_dec_float_100(d.as_const_base_int_ref());
            // TODO : check validity
            return f;
        }
        else
            IMPLEMENT;
    }
    
    Valuable Fraction::operator()(const Variable& v, const Valuable& augmentation) const
    {
        return (augmentation * denominator() - numerator())(v);
    }
    
    omnn::math::Fraction Fraction::Reciprocal() const
    {
        return Fraction(denominator(), numerator());
    }

    bool Fraction::IsSimple() const
    {
        return numerator().IsInt()
            && denominator().IsInt()
        ;
    }
}}

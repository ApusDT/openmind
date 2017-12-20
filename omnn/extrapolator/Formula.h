//
// Created by Сергей Кривонос on 30.09.17.
//

#pragma once
#include <functional>
#include <map>
#include "Product.h"
#include "Variable.h"

namespace omnn {
namespace extrapolator {

    class Sum;
    
class Formula
        : public ValuableDescendantContract<Formula>
{
    using base = ValuableDescendantContract<Formula>;
    friend base;
    
    Variable v;
    Valuable e;
    std::set<Variable> s;
    
    Formula(Valuable&& ex) : e(std::move(ex)) { e.CollectVa(s); }
    Formula(int i) : e(i) { e.CollectVa(s); }
    
    using VaValMap = ::std::map<const Variable,const Valuable*>;
    Valuable GetProductRootByCoordinates(const VaValMap& vaVals) const;
    bool InCoordFactorization(const VaValMap& vaVals) const;
    
protected:
    Formula(const Variable&, const Valuable&);
    std::ostream& print(std::ostream& out) const override;
    virtual Valuable Solve(Valuable& v) const;
public:
    //using f_t = std::function<Valuable&&(Valuable&&)>;
    using base::base;
    static Formula DeduceFormula(const Valuable& e, const Variable& v);
    static Formula DeclareFormula(const Variable& v, const Valuable& e);

    //Formula(const Valuable& e, const f_t& f);
    
    const Variable& getVa() const { return v; }
    const Valuable& getEx() const { return e; }
    
    void optimize() override { e.optimize(); }
    
    template<class... T>
    Valuable operator()(const T&... vl) const
    {
        Valuable root;
        auto copy = e;
        std::initializer_list<Valuable> args = {vl...};
        
        // va values map
        VaValMap vaVals;
        auto vit = s.begin();
        for(auto& a:args){
            auto va = vit++;
            vaVals[*va] = &a;
        }

        if (InCoordFactorization(vaVals))
        {
            root = GetProductRootByCoordinates(vaVals);
            return root;
        }

        vit = s.begin();
        for(auto v:args)
        {
            auto va = vit++;
            copy.Eval(*va, v);
        }
        return Solve(copy);
    }
    
private:
    //f_t f;
};
}}

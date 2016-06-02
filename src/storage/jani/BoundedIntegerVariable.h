#pragma once

#include "src/storage/jani/Variable.h"
#include "src/storage/expressions/Expression.h"

namespace storm {
    namespace jani {
        
        class BoundedIntegerVariable : public Variable {
        public:
            /*!
             * Creates a bounded integer variable.
             */
            BoundedIntegerVariable(std::string const& name, storm::expressions::Variable const& variable, storm::expressions::Expression const& lowerBound, storm::expressions::Expression const& upperBound);
            
            /*!
             * Retrieves the expression defining the lower bound of the variable.
             */
            storm::expressions::Expression const& getLowerBound() const;

            /*!
             * Sets a new lower bound of the variable.
             */
            void setLowerBound(storm::expressions::Expression const& expression);
            
            /*!
             * Retrieves the expression defining the upper bound of the variable.
             */
            storm::expressions::Expression const& getUpperBound() const;

            /*!
             * Sets a new upper bound of the variable.
             */
            void setUpperBound(storm::expressions::Expression const& expression);

            virtual bool isBoundedIntegerVariable() const override;

        private:
            // The expression defining the lower bound of the variable.
            storm::expressions::Expression lowerBound;
            
            // The expression defining the upper bound of the variable.
            storm::expressions::Expression upperBound;
        };
        
    }
}
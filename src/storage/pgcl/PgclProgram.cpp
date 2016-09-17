#include "PgclProgram.h"
#include "StatementPrinterVisitor.h"
#include <typeinfo>

namespace storm {
    namespace pgcl {
        PgclProgram::PgclProgram(vector const& statements, vector const& locationToStatement, std::vector<storm::expressions::Variable> const& parameters, std::shared_ptr<storm::expressions::ExpressionManager> expressions, bool hasLoop, bool hasNondet, bool hasObserve) :
            PgclBlock(statements,
                      expressions, hasLoop, hasNondet, hasObserve),
            locationToStatement(locationToStatement)
        {
        }

        
        vector PgclProgram::getLocationToStatementVector() {
            return this->locationToStatement;
        }
        
        std::vector<storm::expressions::Variable> PgclProgram::getVariables() const {
            std::vector<storm::expressions::Variable> vars;
            for(auto const& v : *(this->getExpressionManager())) {
                vars.push_back(v.first);
            }
            
            return vars;
        }

        std::ostream& operator<<(std::ostream& stream, PgclProgram& program) {
            storm::pgcl::StatementPrinterVisitor printer(stream);
            for(iterator statement = program.begin(); statement != program.end(); statement++) {
                (*statement)->accept(printer);
            }
            return stream;
        }
    }
}
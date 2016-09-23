#include "src/generator/VariableInformation.h"

#include "src/storage/prism/Program.h"
#include "src/storage/jani/Model.h"

#include "src/utility/macros.h"
#include "src/exceptions/InvalidArgumentException.h"

#include <cmath>

namespace storm {
    namespace generator {
        
        BooleanVariableInformation::BooleanVariableInformation(storm::expressions::Variable const& variable, uint_fast64_t bitOffset, bool global) : variable(variable), bitOffset(bitOffset), global(global) {
            // Intentionally left empty.
        }
        
        IntegerVariableInformation::IntegerVariableInformation(storm::expressions::Variable const& variable, int_fast64_t lowerBound, int_fast64_t upperBound, uint_fast64_t bitOffset, uint_fast64_t bitWidth, bool global) : variable(variable), lowerBound(lowerBound), upperBound(upperBound), bitOffset(bitOffset), bitWidth(bitWidth), global(global) {
            // Intentionally left empty.
        }
        
        LocationVariableInformation::LocationVariableInformation(uint64_t highestValue, uint_fast64_t bitOffset, uint_fast64_t bitWidth) : highestValue(highestValue), bitOffset(bitOffset), bitWidth(bitWidth) {
            // Intentionally left empty.
        }
        
        VariableInformation::VariableInformation(storm::prism::Program const& program) : totalBitOffset(0) {
            for (auto const& booleanVariable : program.getGlobalBooleanVariables()) {
                booleanVariables.emplace_back(booleanVariable.getExpressionVariable(), totalBitOffset, true);
                ++totalBitOffset;
            }
            for (auto const& integerVariable : program.getGlobalIntegerVariables()) {
                int_fast64_t lowerBound = integerVariable.getLowerBoundExpression().evaluateAsInt();
                int_fast64_t upperBound = integerVariable.getUpperBoundExpression().evaluateAsInt();
                uint_fast64_t bitwidth = static_cast<uint_fast64_t>(std::ceil(std::log2(upperBound - lowerBound + 1)));
                integerVariables.emplace_back(integerVariable.getExpressionVariable(), lowerBound, upperBound, totalBitOffset, bitwidth, true);
                totalBitOffset += bitwidth;
            }
            for (auto const& module : program.getModules()) {
                for (auto const& booleanVariable : module.getBooleanVariables()) {
                    booleanVariables.emplace_back(booleanVariable.getExpressionVariable(), totalBitOffset);
                    ++totalBitOffset;
                }
                for (auto const& integerVariable : module.getIntegerVariables()) {
                    int_fast64_t lowerBound = integerVariable.getLowerBoundExpression().evaluateAsInt();
                    int_fast64_t upperBound = integerVariable.getUpperBoundExpression().evaluateAsInt();
                    uint_fast64_t bitwidth = static_cast<uint_fast64_t>(std::ceil(std::log2(upperBound - lowerBound + 1)));
                    integerVariables.emplace_back(integerVariable.getExpressionVariable(), lowerBound, upperBound, totalBitOffset, bitwidth);
                    totalBitOffset += bitwidth;
                }
            }
            
            sortVariables();
        }
        
        VariableInformation::VariableInformation(storm::jani::Model const& model) : totalBitOffset(0) {
            // Check that the model does not contain non-transient unbounded integer or non-transient real variables.
            STORM_LOG_THROW(!model.getGlobalVariables().containsNonTransientRealVariables(), storm::exceptions::InvalidArgumentException, "Cannot build model from JANI model that contains global non-transient real variables.");
            STORM_LOG_THROW(!model.getGlobalVariables().containsNonTransientUnboundedIntegerVariables(), storm::exceptions::InvalidArgumentException, "Cannot build model from JANI model that contains non-transient unbounded integer variables.");
            for (auto const& automaton : model.getAutomata()) {
                STORM_LOG_THROW(!automaton.getVariables().containsNonTransientUnboundedIntegerVariables(), storm::exceptions::InvalidArgumentException, "Cannot build model from JANI model that contains non-transient unbounded integer variables in automaton '" << automaton.getName() << "'.");
                STORM_LOG_THROW(!automaton.getVariables().containsNonTransientRealVariables(), storm::exceptions::InvalidArgumentException, "Cannot build model from JANI model that contains non-transient real variables in automaton '" << automaton.getName() << "'.");
            }
            
            for (auto const& variable : model.getGlobalVariables().getBooleanVariables()) {
                booleanVariables.emplace_back(variable.getExpressionVariable(), totalBitOffset, true);
                ++totalBitOffset;
            }
            for (auto const& variable : model.getGlobalVariables().getBoundedIntegerVariables()) {
                int_fast64_t lowerBound = variable.getLowerBound().evaluateAsInt();
                int_fast64_t upperBound = variable.getUpperBound().evaluateAsInt();
                uint_fast64_t bitwidth = static_cast<uint_fast64_t>(std::ceil(std::log2(upperBound - lowerBound + 1)));
                integerVariables.emplace_back(variable.getExpressionVariable(), lowerBound, upperBound, totalBitOffset, bitwidth, true);
                totalBitOffset += bitwidth;
            }
            for (auto const& automaton : model.getAutomata()) {
                uint_fast64_t bitwidth = static_cast<uint_fast64_t>(std::ceil(std::log2(automaton.getNumberOfLocations())));
                locationVariables.emplace_back(automaton.getNumberOfLocations() - 1, totalBitOffset, bitwidth);
                totalBitOffset += bitwidth;
                
                for (auto const& variable : automaton.getVariables().getBooleanVariables()) {
                    booleanVariables.emplace_back(variable.getExpressionVariable(), totalBitOffset);
                    ++totalBitOffset;
                }
                for (auto const& variable : automaton.getVariables().getBoundedIntegerVariables()) {
                    int_fast64_t lowerBound = variable.getLowerBound().evaluateAsInt();
                    int_fast64_t upperBound = variable.getUpperBound().evaluateAsInt();
                    uint_fast64_t bitwidth = static_cast<uint_fast64_t>(std::ceil(std::log2(upperBound - lowerBound + 1)));
                    integerVariables.emplace_back(variable.getExpressionVariable(), lowerBound, upperBound, totalBitOffset, bitwidth);
                    totalBitOffset += bitwidth;
                }
            }
            
            sortVariables();
        }
        
        uint_fast64_t VariableInformation::getTotalBitOffset(bool roundTo64Bit) const {
            uint_fast64_t result = totalBitOffset;
            if (roundTo64Bit) {
                result = ((result >> 6) + 1) << 6;
            }
            return result;
        }
        
        void VariableInformation::sortVariables() {
            // Sort the variables so we can make some assumptions when iterating over them (in the next-state generators).
            std::sort(booleanVariables.begin(), booleanVariables.end(), [] (BooleanVariableInformation const& a, BooleanVariableInformation const& b) { return a.variable < b.variable; } );
            std::sort(integerVariables.begin(), integerVariables.end(), [] (IntegerVariableInformation const& a, IntegerVariableInformation const& b) { return a.variable < b.variable; });
        }
    }
}

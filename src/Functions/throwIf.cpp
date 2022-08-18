#include <Functions/IFunction.h>
#include <Functions/FunctionFactory.h>
#include <Functions/FunctionHelpers.h>
#include <Columns/ColumnString.h>
#include <Columns/ColumnsNumber.h>
#include <Columns/ColumnsCommon.h>
#include <Common/ErrorCodes.h>
#include <DataTypes/DataTypesNumber.h>
#include <IO/WriteHelpers.h>
#include <Interpreters/Context.h>

namespace DB
{
namespace ErrorCodes
{
    extern const int ILLEGAL_COLUMN;
    extern const int ILLEGAL_TYPE_OF_ARGUMENT;
    extern const int NUMBER_OF_ARGUMENTS_DOESNT_MATCH;
    extern const int FUNCTION_THROW_IF_VALUE_IS_NON_ZERO;
}

namespace
{

/// The regex-based code style check script in CI complains when it sees "ErrorCodes:: ErrorCode" (space added to avoid another match).
/// Because this expression is only used in this file, don't add some suppression mechanism to the already complex style checker, instead
/// work around by creating a namespace alias.
namespace ErrorCodeAlias = ErrorCodes;

/// Throw an exception if the argument is non zero.
class FunctionThrowIf : public IFunction, WithContext
{
public:
    static constexpr auto name = "throwIf";

    static FunctionPtr create(ContextPtr context_)
    {
        return std::make_shared<FunctionThrowIf>(context_);
    }

    explicit FunctionThrowIf(ContextPtr context_) : WithContext(context_) {}
    String getName() const override { return name; }
    bool isVariadic() const override { return true; }
    bool isSuitableForShortCircuitArgumentsExecution(const DataTypesWithConstInfo & /*arguments*/) const override { return true; }
    size_t getNumberOfArguments() const override { return 0; }

    DataTypePtr getReturnTypeImpl(const DataTypes & arguments) const override
    {
        const size_t number_of_arguments = arguments.size();

        const auto & settings = getContext()->getSettingsRef();
        bool allow_custom_error_code_argument = settings.allow_custom_error_code_in_throwif;

        if (number_of_arguments < 1 || number_of_arguments > (allow_custom_error_code_argument ? 3 : 2))
            throw Exception(ErrorCodes::NUMBER_OF_ARGUMENTS_DOESNT_MATCH,
                "Number of arguments for function {} doesn't match: passed {}, should be {}",
                getName(), toString(number_of_arguments), allow_custom_error_code_argument ? "1 or 2 or 3" : "1 or 2");

        if (!isNativeNumber(arguments[0]))
            throw Exception(ErrorCodes::ILLEGAL_TYPE_OF_ARGUMENT,
                "First argument of function {} must be a number (passed: {})", getName(), arguments[0]->getName());

        if (number_of_arguments > 1 && !isString(arguments[1]))
            throw Exception(ErrorCodes::ILLEGAL_TYPE_OF_ARGUMENT,
                "Second argument of function {} must be a string (passed: {})", getName(), arguments[1]->getName());

        if (allow_custom_error_code_argument)
            if (number_of_arguments > 2 && !isNumber(arguments[2]))
                throw Exception(ErrorCodes::ILLEGAL_TYPE_OF_ARGUMENT,
                    "Third argument of function {} must be a number (passed: {})", getName(), arguments[2]->getName());


        return std::make_shared<DataTypeUInt8>();
    }

    bool useDefaultImplementationForConstants() const override { return false; }
    ColumnNumbers getArgumentsThatAreAlwaysConstant() const override { return {1, 2}; }

    /** Prevent constant folding for FunctionThrowIf because for short circuit evaluation
      * it is unsafe to evaluate this function during DAG analysis.
      */
    bool isSuitableForConstantFolding() const override { return false; }

    ColumnPtr executeImpl(const ColumnsWithTypeAndName & arguments, const DataTypePtr & result_type, size_t input_rows_count) const override
    {
        if (input_rows_count == 0)
            return result_type->createColumn();

        std::optional<String> custom_message;
        if (arguments.size() == 2)
        {
            const auto * message_column = checkAndGetColumnConst<ColumnString>(arguments[1].column.get());
            if (!message_column)
                throw Exception(ErrorCodes::ILLEGAL_COLUMN, "Second argument for function {} must be constant String", getName());

            custom_message = message_column->getValue<String>();
        }

        std::optional<ErrorCodeAlias::ErrorCode> custom_error_code;
        if (arguments.size() == 3)
        {
            if (!isColumnConst(*(arguments[2].column)))
                throw Exception(ErrorCodes::ILLEGAL_COLUMN, "Third argument for function {} must be constant number", getName());

            custom_error_code = arguments[2].column->getUInt(0);
        }

        auto first_argument_column = arguments.front().column;
        const auto * in = first_argument_column.get();

        ColumnPtr res;
        if (!((res = execute<UInt8>(in, custom_message, custom_error_code))
            || (res = execute<UInt16>(in, custom_message, custom_error_code))
            || (res = execute<UInt32>(in, custom_message, custom_error_code))
            || (res = execute<UInt64>(in, custom_message, custom_error_code))
            || (res = execute<Int8>(in, custom_message, custom_error_code))
            || (res = execute<Int16>(in, custom_message, custom_error_code))
            || (res = execute<Int32>(in, custom_message, custom_error_code))
            || (res = execute<Int64>(in, custom_message, custom_error_code))
            || (res = execute<Float32>(in, custom_message, custom_error_code))
            || (res = execute<Float64>(in, custom_message, custom_error_code))))
        {
            throw Exception(ErrorCodes::ILLEGAL_COLUMN, "Illegal column {} of first argument of function {}", in->getName(), getName());
        }

        return res;
    }

private:
    template <typename T>
    ColumnPtr execute(const IColumn * in_untyped, const std::optional<String> & message, const std::optional<ErrorCodeAlias::ErrorCode> & error_code) const
    {
        const auto * in = checkAndGetColumn<ColumnVector<T>>(in_untyped);

        if (!in)
            in = checkAndGetColumnConstData<ColumnVector<T>>(in_untyped);

        if (in)
        {
            const auto & in_data = in->getData();
            if (!memoryIsZero(in_data.data(), 0, in_data.size() * sizeof(in_data[0])))
            {
                throw Exception(
                    error_code.value_or(ErrorCodes::FUNCTION_THROW_IF_VALUE_IS_NON_ZERO),
                    message.value_or("Value passed to '" + getName() + "' function is non-zero"));
            }

            size_t result_size = in_untyped->size();

            /// We return non constant to avoid constant folding.
            return ColumnUInt8::create(result_size, 0);
        }

        return nullptr;
    }
};

}

REGISTER_FUNCTION(ThrowIf)
{
    factory.registerFunction<FunctionThrowIf>();
}

}

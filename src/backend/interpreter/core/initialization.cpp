#include "initialization.h"
#include "../../../common/ast.h"
#include "../../../common/debug.h"
#include "../../../frontend/recursive_parser/recursive_parser.h"
#include "evaluator/core/evaluator.h"
#include "executors/control_flow_executor.h"
#include "executors/statement_executor.h"
#include "executors/statement_list_executor.h"
#include "handlers/control/assertion.h"
#include "handlers/control/break_continue.h"
#include "handlers/control/return.h"
#include "handlers/declarations/function.h"
#include "handlers/declarations/impl.h"
#include "handlers/declarations/interface.h"
#include "handlers/declarations/struct.h"
#include "handlers/statements/expression.h"
#include "interpreter.h"
#include "managers/arrays/manager.h"
#include "managers/common/global_init.h"
#include "managers/common/operations.h"
#include "managers/structs/assignment.h"
#include "managers/structs/member_variables.h"
#include "managers/structs/operations.h"
#include "managers/structs/sync.h"
#include "managers/types/enums.h"
#include "managers/types/interfaces.h"
#include "managers/types/manager.h"
#include "managers/variables/manager.h"
#include "managers/variables/static.h"
#include "output/output_manager.h"
#include "services/array_processing_service.h"
#include "services/expression_service.h"
#include "services/variable_access_service.h"

// ========================================================================
// コンストラクタ - 全マネージャーとサービスを初期化
// ========================================================================
Interpreter::Interpreter(bool debug)
    : debug_mode(debug), output_manager_(std::make_unique<OutputManager>(this)),
      variable_manager_(std::make_unique<VariableManager>(this)),
      type_manager_(std::make_unique<TypeManager>(this)) {

    // ExpressionEvaluatorを最初に初期化
    expression_evaluator_ = std::make_unique<ExpressionEvaluator>(*this);

    // ArrayManagerはVariableManagerとExpressionEvaluatorが必要なので後で初期化
    array_manager_ = std::make_unique<ArrayManager>(
        variable_manager_.get(), expression_evaluator_.get(), this);

    // StatementExecutorを初期化
    statement_executor_ = std::make_unique<StatementExecutor>(*this);

    // CommonOperationsを初期化（他のManagerが必要なので最後に初期化）
    common_operations_ = std::make_unique<CommonOperations>(this);

    // DRY効率化: 統一変数アクセスサービスを初期化
    variable_access_service_ = std::make_unique<VariableAccessService>(this);

    // DRY効率化: 統一式評価サービスを初期化
    expression_service_ = std::make_unique<ExpressionService>(this);

    // DRY効率化: 統一配列処理サービスを初期化
    array_processing_service_ = std::make_unique<ArrayProcessingService>(
        this, common_operations_.get());

    // enum管理サービスを初期化
    enum_manager_ = std::make_unique<EnumManager>();

    // static変数管理サービスを初期化
    static_variable_manager_ = std::make_unique<StaticVariableManager>(this);

    // interface/impl管理サービスを初期化
    interface_operations_ = std::make_unique<InterfaceOperations>(this);

    // struct操作管理サービスを初期化
    struct_operations_ = std::make_unique<StructOperations>(this);

    // struct変数管理サービスを初期化
    struct_variable_manager_ = std::make_unique<StructVariableManager>(this);

    // struct代入管理サービスを初期化
    struct_assignment_manager_ =
        std::make_unique<StructAssignmentManager>(this);

    // struct同期管理サービスを初期化
    struct_sync_manager_ = std::make_unique<StructSyncManager>(this);

    // グローバル初期化管理サービスを初期化
    global_initialization_manager_ =
        std::make_unique<GlobalInitializationManager>(this);

    // 制御フロー実行サービスを初期化
    control_flow_executor_ = std::make_unique<ControlFlowExecutor>(this);

    // 文リスト・複合文実行サービスを初期化
    statement_list_executor_ = std::make_unique<StatementListExecutor>(this);

    // return文処理サービスを初期化
    return_handler_ = std::make_unique<ReturnHandler>(this);

    // アサーション文処理サービスを初期化
    assertion_handler_ = std::make_unique<AssertionHandler>(this);

    // break/continue文処理サービスを初期化
    break_continue_handler_ = std::make_unique<BreakContinueHandler>(this);

    // 関数宣言処理サービスを初期化
    function_declaration_handler_ =
        std::make_unique<FunctionDeclarationHandler>(this);

    // 構造体宣言処理サービスを初期化
    struct_declaration_handler_ =
        std::make_unique<StructDeclarationHandler>(this);

    // インターフェース宣言処理サービスを初期化
    interface_declaration_handler_ =
        std::make_unique<InterfaceDeclarationHandler>(this);

    // impl宣言処理サービスを初期化
    impl_declaration_handler_ = std::make_unique<ImplDeclarationHandler>(this);

    // 式文処理サービスを初期化
    expression_statement_handler_ =
        std::make_unique<ExpressionStatementHandler>(this);

    // グローバルスコープを初期化
    // ネストされた関数呼び出しに備えて容量を予約（再割り当てを防ぐ）
    scope_stack.reserve(64);
    scope_stack.push_back(global_scope);
}

// ========================================================================
// グローバル変数初期化
// ========================================================================
void Interpreter::initialize_global_variables(const ASTNode *node) {
    global_initialization_manager_->initialize_global_variables(node);
}

// ========================================================================
// enum定義の同期
// ========================================================================
void Interpreter::sync_enum_definitions_from_parser(RecursiveParser *parser) {
    global_initialization_manager_->sync_enum_definitions_from_parser(parser);
}

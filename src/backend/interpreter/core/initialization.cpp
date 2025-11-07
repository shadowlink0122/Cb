#include "initialization.h"
#include "../../../common/ast.h"
#include "../../../common/debug.h"
#include "../../../frontend/recursive_parser/recursive_parser.h"
#include "evaluator/core/evaluator.h"
#include "event_loop/event_loop.h"
#include "event_loop/simple_event_loop.h" // v0.13.0 Phase 2.0
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
#include "services/debug_service.h"
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

    // v0.12.0: Event Loop を初期化
    event_loop_ = std::make_unique<cb::EventLoop>();

    // v0.13.0 Phase 2.0: SimpleEventLoop を初期化
    simple_event_loop_ = std::make_unique<cb::SimpleEventLoop>(*this);

    // グローバルスコープを初期化
    // ネストされた関数呼び出しに備えて容量を予約（再割り当てを防ぐ）
    scope_stack.reserve(64);
    scope_stack.push_back(global_scope);

    // v0.10.0: デストラクタスタックをグローバルスコープ用に初期化
    destructor_stacks_.push_back(
        std::vector<std::pair<std::string, std::string>>());

    // v0.11.0: 組み込み型（Option<T>, Result<T, E>）の初期化
    initialize_builtin_types();
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

// v0.11.0: Parserからinterface/impl定義を同期
void Interpreter::sync_interface_definitions_from_parser(
    RecursiveParser *parser) {
    if (!parser)
        return;

    const auto &interface_defs = parser->get_interface_definitions();
    for (const auto &pair : interface_defs) {
        interface_operations_->register_interface_definition(pair.first,
                                                             pair.second);

        if (debug_mode) {
            std::cerr << "[SYNC_INTERFACE] " << pair.first << " with "
                      << pair.second.methods.size() << " methods" << std::endl;
        }
    }
}

void Interpreter::sync_impl_definitions_from_parser(RecursiveParser *parser) {
    if (!parser)
        return;

    // v0.11.0: Parserからimpl_definitions_のコピーを作成（後でクリアするため）
    const auto &impl_defs_ref = parser->get_impl_definitions();
    std::vector<ImplDefinition> impl_defs(impl_defs_ref.begin(),
                                          impl_defs_ref.end());

    if (debug_mode) {
        std::cerr << "[SYNC_IMPL] Total impl definitions: " << impl_defs.size()
                  << std::endl;
    }

    // v0.11.0: Parserからimpl_nodes_の所有権を転送（use-after-free対策）
    auto &parser_impl_nodes = parser->get_impl_nodes_for_transfer();
    if (!parser_impl_nodes.empty()) {
        if (debug_mode) {
            std::cerr << "[SYNC_IMPL] Transferring " << parser_impl_nodes.size()
                      << " impl nodes from parser to interpreter" << std::endl;
        }

        // 所有権を移動
        for (auto &node : parser_impl_nodes) {
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "[SYNC_IMPL] Transferring impl_node=%p, "
                         "arguments.size()=%zu",
                         (void *)node.get(), node->arguments.size());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
            impl_nodes_.push_back(std::move(node));
        }
        parser_impl_nodes.clear();

        {
            char dbg_buf[512];
            snprintf(
                dbg_buf, sizeof(dbg_buf),
                "[SYNC_IMPL] After transfer, interpreter has %zu impl_nodes",
                impl_nodes_.size());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
    }

    // v0.11.0:
    // Parserのimpl_definitions_をクリア（Parser破棄時のuse-after-free対策）
    // これらはInterpreterに転送されるので、Parser側では不要
    auto &parser_impl_defs = parser->get_impl_definitions_for_clear();
    {
        char dbg_buf[512];
        snprintf(dbg_buf, sizeof(dbg_buf),
                 "[SYNC_IMPL] Clearing %zu impl_definitions from parser",
                 parser_impl_defs.size());
        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
    }
    parser_impl_defs.clear();

    // v0.11.0: impl_defのimpl_nodeポインタを転送されたノードに更新
    // （Parserのノードを指したままだと、Parser破棄時にuse-after-freeが発生）
    // node_idxは今回転送されたノードの開始位置から始める
    size_t node_idx = impl_nodes_.size() - impl_defs.size();
    for (size_t i = 0; i < impl_defs.size(); ++i) {
        auto impl_def = impl_defs[i]; // コピー（ポインタを更新するため）

        // impl_nodeをInterpreterの所有するノードに更新
        if (node_idx < impl_nodes_.size()) {
            impl_def.impl_node = impl_nodes_[node_idx].get();

            if (debug_mode) {
                std::cerr << "[SYNC_IMPL] [" << i
                          << "] Updated impl_node to interpreter's node: "
                          << (void *)impl_def.impl_node << std::endl;
            }

            // methods/constructors/destructorも更新されたノードから取得
            impl_def.methods.clear();
            impl_def.constructors.clear();
            impl_def.destructor = nullptr;
            for (const auto &arg : impl_def.impl_node->arguments) {
                if (arg->node_type == ASTNodeType::AST_FUNC_DECL) {
                    impl_def.methods.push_back(arg.get());
                } else if (arg->node_type ==
                           ASTNodeType::AST_CONSTRUCTOR_DECL) {
                    impl_def.constructors.push_back(arg.get());

                    // struct_constructors_にも登録
                    struct_constructors_[impl_def.struct_name].push_back(
                        arg.get());

                    if (debug_mode) {
                        std::cerr << "[SYNC_IMPL]   Extracted constructor at "
                                  << (void *)arg.get()
                                  << ", registered to struct_constructors_["
                                  << impl_def.struct_name << "]" << std::endl;
                    }
                } else if (arg->node_type == ASTNodeType::AST_DESTRUCTOR_DECL) {
                    impl_def.destructor = arg.get();

                    // struct_destructors_にも登録
                    struct_destructors_[impl_def.struct_name] = arg.get();

                    if (debug_mode) {
                        std::cerr << "[SYNC_IMPL]   Extracted destructor for "
                                  << impl_def.struct_name << std::endl;
                    }
                }
            }

            node_idx++;
        }

        interface_operations_->register_impl_definition(impl_def);

        if (debug_mode) {
            std::cerr << "[SYNC_IMPL] [" << i << "] " << impl_def.interface_name
                      << " for " << impl_def.struct_name;
            std::cerr << ", impl_node=" << (void *)impl_def.impl_node;
            std::cerr << ", methods=" << impl_def.methods.size() << std::endl;
        }

        // メソッドを登録（ASTノードから）
        // implのメソッドをグローバル関数として登録する必要がある
        // これにはASTノードが必要なので、Parser側から取得する必要がある
    }
}

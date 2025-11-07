#ifndef CB_INTERPRETER_TYPES_FUTURE_H
#define CB_INTERPRETER_TYPES_FUTURE_H

#include <memory>
#include <string>

namespace cb {

// Future<T>の値型
enum class FutureValueType {
    INT,
    LONG,
    DOUBLE,
    STRING,
    VOID // sleep_ms()などで使用
};

// Future<T>の実装クラス（プリミティブ型のみサポート）
class FutureValue {
  public:
    FutureValue(FutureValueType type);
    ~FutureValue();

    // 値の設定（型に応じて呼び分ける）
    void set_value_int(int value);
    void set_value_long(long value);
    void set_value_double(double value);
    void set_value_string(const std::string &value);
    void set_ready(); // void型の場合

    // 値の取得（型に応じて呼び分ける）
    int get_value_int() const;
    long get_value_long() const;
    double get_value_double() const;
    std::string get_value_string() const;

    // 状態確認
    bool is_ready() const { return is_ready_; }
    FutureValueType get_type() const { return type_; }

    // コピー・ムーブ禁止（Futureは一度きりの値を持つ）
    FutureValue(const FutureValue &) = delete;
    FutureValue &operator=(const FutureValue &) = delete;
    FutureValue(FutureValue &&) = default;
    FutureValue &operator=(FutureValue &&) = default;

  private:
    FutureValueType type_;
    bool is_ready_;
    void *value_; // プリミティブ型の値を保持

    void free_value(); // デストラクタで値を解放
};

} // namespace cb

#endif // CB_INTERPRETER_TYPES_FUTURE_H

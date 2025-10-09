# Cb言語 サンプルコード集

実践的なCb言語のサンプルコード集です。

## 目次

1. [数値計算](#1-数値計算)
2. [文字列処理](#2-文字列処理)
3. [配列操作](#3-配列操作)
4. [アルゴリズム](#4-アルゴリズム)
5. [データ構造](#5-データ構造)
6. [ポインタ活用](#6-ポインタ活用)
7. [実用的なプログラム](#7-実用的なプログラム)

---

## 1. 数値計算

### 最大公約数（GCD）

```cb
int gcd(int a, int b) {
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

int main() {
    int result = gcd(48, 18);
    println("GCD(48, 18) =", result);  // 6
    return 0;
}
```

### エラトステネスの篩（素数列挙）

```cb
// エラトステネスの篩で素数を列挙
void sieve_of_eratosthenes(int n) {
    bool[101] is_prime;  // n+1個の要素（最大100まで）
    
    // 初期化: 全てtrueにする
    for (int i = 0; i <= n; i++) {
        is_prime[i] = true;
    }
    
    // 0と1は素数ではない
    is_prime[0] = false;
    is_prime[1] = false;
    
    // エラトステネスの篩
    for (int i = 2; i * i <= n; i++) {
        if (is_prime[i]) {
            // iの倍数を全て素数でないとマーク
            for (int j = i * i; j <= n; j = j + i) {
                is_prime[j] = false;
            }
        }
    }
    
    // 素数を出力
    println("Prime numbers up to", n, ":");
    for (int i = 2; i <= n; i++) {
        if (is_prime[i]) {
            println(i);
        }
    }
}

int main() {
    sieve_of_eratosthenes(100);
    return 0;
}
```

### 累乗計算

```cb
int power(int base, int exp) {
    int result = 1;
    for (int i = 0; i < exp; i++) {
        result *= base;
    }
    return result;
}

int main() {
    println("2^10 =", power(2, 10));  // 1024
    println("3^5 =", power(3, 5));    // 243
    return 0;
}
```

---

## 2. 文字列処理

### 文字列の長さ計算

```cb
int string_length(string str) {
    int length = 0;
    // 実際の実装はインタープリター側で処理
    return length;
}

int main() {
    string text = "Hello, World!";
    println("Text:", text);
    return 0;
}
```

### 大文字・小文字変換（ASCII）

```cb
char to_upper(char c) {
    if (c >= 'a' && c <= 'z') {
        return c - 32;
    }
    return c;
}

char to_lower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

int main() {
    char ch = 'a';
    println("Upper:", to_upper(ch));  // A
    
    ch = 'Z';
    println("Lower:", to_lower(ch));  // z
    
    return 0;
}
```

---

## 3. 配列操作

### 配列の合計値

```cb
int array_sum(int[5] arr) {
    int sum = 0;
    for (int i = 0; i < 5; i++) {
        sum += arr[i];
    }
    return sum;
}

int main() {
    int[5] numbers = [10, 20, 30, 40, 50];
    int total = array_sum(numbers);
    println("Sum:", total);  // 150
    return 0;
}
```

### 配列の最大値・最小値

```cb
int find_max(int[10] arr) {
    int max = arr[0];
    for (int i = 1; i < 10; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    return max;
}

int find_min(int[10] arr) {
    int min = arr[0];
    for (int i = 1; i < 10; i++) {
        if (arr[i] < min) {
            min = arr[i];
        }
    }
    return min;
}

int main() {
    int[10] data = [45, 12, 89, 23, 67, 34, 91, 56, 78, 10];
    
    println("Max:", find_max(data));  // 91
    println("Min:", find_min(data));  // 10
    
    return 0;
}
```

### 配列の反転

```cb
void reverse_array(int[5]& arr) {
    int left = 0;
    int right = 4;
    
    while (left < right) {
        int temp = arr[left];
        arr[left] = arr[right];
        arr[right] = temp;
        left++;
        right--;
    }
}

int main() {
    int[5] numbers = [1, 2, 3, 4, 5];
    
    println("Original:");
    for (int i = 0; i < 5; i++) {
        print("%d ", numbers[i]);
    }
    println("");
    
    reverse_array(numbers);
    
    println("Reversed:");
    for (int i = 0; i < 5; i++) {
        print("%d ", numbers[i]);
    }
    println("");
    
    return 0;
}
```

---

## 4. アルゴリズム

### バブルソート

```cb
void bubble_sort(int[10]& arr) {
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9 - i; j++) {
            if (arr[j] > arr[j + 1]) {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

int main() {
    int[10] data = [64, 34, 25, 12, 22, 11, 90, 88, 45, 50];
    
    println("Before sort:");
    for (int i = 0; i < 10; i++) {
        print("%d ", data[i]);
    }
    println("");
    
    bubble_sort(data);
    
    println("After sort:");
    for (int i = 0; i < 10; i++) {
        print("%d ", data[i]);
    }
    println("");
    
    return 0;
}
```

### 二分探索

```cb
int binary_search(int[10] arr, int target) {
    int left = 0;
    int right = 9;
    
    while (left <= right) {
        int mid = (left + right) / 2;
        
        if (arr[mid] == target) {
            return mid;
        } else if (arr[mid] < target) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    
    return -1;  // 見つからない
}

int main() {
    // ソート済み配列
    int[10] data = [1, 3, 5, 7, 9, 11, 13, 15, 17, 19];
    
    int target = 13;
    int index = binary_search(data, target);
    
    if (index != -1) {
        println("Found at index:", index);
    } else {
        println("Not found");
    }
    
    return 0;
}
```

### フィボナッチ数列（反復版）

```cb
int fibonacci_iterative(int n) {
    if (n <= 1) {
        return n;
    }
    
    int prev = 0;
    int curr = 1;
    
    for (int i = 2; i <= n; i++) {
        int next = prev + curr;
        prev = curr;
        curr = next;
    }
    
    return curr;
}

int main() {
    for (int i = 0; i < 15; i++) {
        println("fib(", i, ") =", fibonacci_iterative(i));
    }
    return 0;
}
```

---

## 5. データ構造

### スタック（配列実装）

```cb
struct Stack {
    int[100] data;
    int top;
};

void stack_init(Stack& s) {
    s.top = -1;
}

bool stack_is_empty(Stack& s) {
    return s.top == -1;
}

bool stack_is_full(Stack& s) {
    return s.top == 99;
}

void stack_push(Stack& s, int value) {
    if (!stack_is_full(s)) {
        s.top++;
        s.data[s.top] = value;
    }
}

int stack_pop(Stack& s) {
    if (!stack_is_empty(s)) {
        int value = s.data[s.top];
        s.top--;
        return value;
    }
    return -1;
}

int main() {
    Stack s;
    stack_init(s);
    
    stack_push(s, 10);
    stack_push(s, 20);
    stack_push(s, 30);
    
    println(stack_pop(s));  // 30
    println(stack_pop(s));  // 20
    println(stack_pop(s));  // 10
    
    return 0;
}
```

### キュー（配列実装）

```cb
struct Queue {
    int[100] data;
    int front;
    int rear;
    int size;
};

void queue_init(Queue& q) {
    q.front = 0;
    q.rear = -1;
    q.size = 0;
}

bool queue_is_empty(Queue& q) {
    return q.size == 0;
}

bool queue_is_full(Queue& q) {
    return q.size == 100;
}

void queue_enqueue(Queue& q, int value) {
    if (!queue_is_full(q)) {
        q.rear = (q.rear + 1) % 100;
        q.data[q.rear] = value;
        q.size++;
    }
}

int queue_dequeue(Queue& q) {
    if (!queue_is_empty(q)) {
        int value = q.data[q.front];
        q.front = (q.front + 1) % 100;
        q.size--;
        return value;
    }
    return -1;
}

int main() {
    Queue q;
    queue_init(q);
    
    queue_enqueue(q, 10);
    queue_enqueue(q, 20);
    queue_enqueue(q, 30);
    
    println(queue_dequeue(q));  // 10
    println(queue_dequeue(q));  // 20
    println(queue_dequeue(q));  // 30
    
    return 0;
}
```

### スタック（interface/impl実装）

interface/implを使うことで、実装の詳細を隠蔽し、より柔軟な設計が可能になります。

```cb
// スタックのインターフェース定義
interface Stack {
    void push(int value);
    int pop();
    int peek();
    bool is_empty();
    int size();
}

// 配列ベースのスタック構造体
struct ArrayStack {
    int[100] data;
    int top;
}

// インターフェースの実装
impl Stack for ArrayStack {
    void push(int value) {
        if (top < 100) {
            data[top] = value;
            top = top + 1;
        }
    }
    
    int pop() {
        if (top > 0) {
            top = top - 1;
            return data[top];
        }
        return -1;  // エラー値
    }
    
    int peek() {
        if (top > 0) {
            return data[top - 1];
        }
        return -1;
    }
    
    bool is_empty() {
        return top == 0;
    }
    
    int size() {
        return top;
    }
}

void main() {
    ArrayStack stack;
    stack.top = 0;
    
    stack.push(10);
    stack.push(20);
    stack.push(30);
    
    println(stack.peek());  // 30（取り出さない）
    println(stack.pop());   // 30
    println(stack.pop());   // 20
    println(stack.size());  // 1
    println(stack.pop());   // 10
    println(stack.is_empty());  // true
}
```

### キュー（interface/impl実装）

```cb
// キューのインターフェース定義
interface Queue {
    void enqueue(int value);
    int dequeue();
    int front();
    bool is_empty();
    int size();
}

// 配列ベースのキュー構造体（循環バッファ）
struct ArrayQueue {
    int[100] data;
    int head;
    int tail;
    int count;
}

// インターフェースの実装
impl Queue for ArrayQueue {
    void enqueue(int value) {
        if (count < 100) {
            data[tail] = value;
            tail = (tail + 1) % 100;  // 循環バッファ
            count = count + 1;
        }
    }
    
    int dequeue() {
        if (count > 0) {
            int value = data[head];
            head = (head + 1) % 100;
            count = count - 1;
            return value;
        }
        return -1;  // エラー値
    }
    
    int front() {
        if (count > 0) {
            return data[head];
        }
        return -1;
    }
    
    bool is_empty() {
        return count == 0;
    }
    
    int size() {
        return count;
    }
}

void main() {
    ArrayQueue queue;
    queue.head = 0;
    queue.tail = 0;
    queue.count = 0;
    
    queue.enqueue(10);
    queue.enqueue(20);
    queue.enqueue(30);
    
    println(queue.front());    // 10（取り出さない）
    println(queue.dequeue());  // 10
    println(queue.dequeue());  // 20
    println(queue.size());     // 1
    println(queue.dequeue());  // 30
    println(queue.is_empty()); // true
}
```

**interface/implの利点**:
- インターフェースと実装を分離できる
- 同じインターフェースで複数の実装を提供可能（例: ArrayStack, LinkedListStack）
- 実装の詳細を隠蔽し、利用者はインターフェースのみを意識すればよい
- ポリモーフィズムによる柔軟な設計が可能

**注意点**:
- `interface`と`impl`の宣言の最後にセミコロンをつけてもエラーにはなりませんが、慣習的につけないことが推奨されます
- `struct`の宣言の最後には必ずセミコロンが必要です
- `impl`ブロック内では、構造体のメンバーに`this.`なしで直接アクセスできます

---

## 6. ポインタ活用

### スワップ関数

```cb
void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

int main() {
    int x = 10;
    int y = 20;
    
    println("Before: x =", x, ", y =", y);
    swap(&x, &y);
    println("After: x =", x, ", y =", y);
    
    return 0;
}
```

### ポインタで配列を走査

```cb
int sum_with_pointer(int* ptr, int size) {
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sum += *(ptr + i);
    }
    return sum;
}

int main() {
    int[5] arr = [10, 20, 30, 40, 50];
    int* ptr = &arr[0];
    
    int total = sum_with_pointer(ptr, 5);
    println("Sum:", total);  // 150
    
    return 0;
}
```

### コールバック関数

```cb
// 5.1 コールバック関数のサンプル
// コールバック関数を受け取る高階関数
int apply_operation(int* operation, int a, int b) {
    return operation(a, b);
}

int add(int a, int b) {
    return a + b;
}

int multiply(int a, int b) {
    return a * b;
}

int main() {
    int result1 = apply_operation(&add, 10, 20);
    println("Add result:", result1);  // 30
    
    int result2 = apply_operation(&multiply, 10, 20);
    println("Multiply result:", result2);  // 200
    
    return 0;
}
```

---

## 7. 実用的なプログラム

### 電卓

```cb
int calculate(char op, int a, int b) {
    if (op == '+') {
        return a + b;
    } else if (op == '-') {
        return a - b;
    } else if (op == '*') {
        return a * b;
    } else if (op == '/') {
        if (b != 0) {
            return a / b;
        } else {
            println("Error: Division by zero");
            return 0;
        }
    }
    return 0;
}

int main() {
    println("10 + 5 =", calculate('+', 10, 5));   // 15
    println("10 - 5 =", calculate('-', 10, 5));   // 5
    println("10 * 5 =", calculate('*', 10, 5));   // 50
    println("10 / 5 =", calculate('/', 10, 5));   // 2
    
    return 0;
}
```

### 成績管理システム

```cb
struct Student {
    string name;
    int[3] scores;  // 3科目の点数
};

int calculate_total(Student& s) {
    int total = 0;
    for (int i = 0; i < 3; i++) {
        total += s.scores[i];
    }
    return total;
}

double calculate_average(Student& s) {
    int total = calculate_total(s);
    return total / 3.0;
}

int main() {
    Student[3] students;
    
    students[0].name = "Alice";
    students[0].scores[0] = 85;
    students[0].scores[1] = 90;
    students[0].scores[2] = 88;
    
    students[1].name = "Bob";
    students[1].scores[0] = 78;
    students[1].scores[1] = 82;
    students[1].scores[2] = 80;
    
    students[2].name = "Charlie";
    students[2].scores[0] = 92;
    students[2].scores[1] = 95;
    students[2].scores[2] = 89;
    
    for (int i = 0; i < 3; i++) {
        println("Student:", students[i].name);
        println("  Total:", calculate_total(students[i]));
        println("  Average:", calculate_average(students[i]));
        println("");
    }
    
    return 0;
}
```

### タイマー（カウントダウン）

```cb
void countdown(int seconds) {
    for (int i = seconds; i > 0; i--) {
        println(i);
        // 実際の待機は実装環境による
    }
    println("Time's up!");
}

int main() {
    countdown(10);
    return 0;
}
```

---

## まとめ

このサンプルコード集では、様々な実践的なCbプログラムを紹介しました。

### 学習のポイント

1. **基本から応用へ**: 簡単な例から徐々に複雑な実装へ
2. **アルゴリズムの理解**: ソートや探索の基本的な考え方
3. **データ構造の活用**: スタック、キューなどの実装方法
4. **ポインタの使い方**: 効率的なメモリ操作とコールバック

### 次のステップ

- これらのコードを実際に実行して動作を確認
- 自分なりの改良や拡張を試す
- より複雑なプログラムに挑戦

### 参考資料

- [基本構文ガイド](basic_syntax_guide.md)
- [言語仕様書](../spec.md)
- [公式サンプル](../../sample/)

Happy Coding! 🚀

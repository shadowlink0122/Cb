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
func int gcd(int a, int b) {
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

func int main() {
    int result = gcd(48, 18);
    println("GCD(48, 18) =", result);  // 6
    return 0;
}
```

### 素数判定

```cb
func bool is_prime(int n) {
    if (n <= 1) {
        return false;
    }
    if (n <= 3) {
        return true;
    }
    if (n % 2 == 0 || n % 3 == 0) {
        return false;
    }
    
    for (int i = 5; i * i <= n; i = i + 6) {
        if (n % i == 0 || n % (i + 2) == 0) {
            return false;
        }
    }
    return true;
}

func int main() {
    for (int i = 2; i <= 30; i++) {
        if (is_prime(i)) {
            println(i);
        }
    }
    return 0;
}
```

### 累乗計算

```cb
func int power(int base, int exp) {
    int result = 1;
    for (int i = 0; i < exp; i++) {
        result *= base;
    }
    return result;
}

func int main() {
    println("2^10 =", power(2, 10));  // 1024
    println("3^5 =", power(3, 5));    // 243
    return 0;
}
```

---

## 2. 文字列処理

### 文字列の長さ計算

```cb
func int string_length(string str) {
    int length = 0;
    // 実際の実装はインタープリター側で処理
    return length;
}

func int main() {
    string text = "Hello, World!";
    println("Text:", text);
    return 0;
}
```

### 大文字・小文字変換（ASCII）

```cb
func char to_upper(char c) {
    if (c >= 'a' && c <= 'z') {
        return c - 32;
    }
    return c;
}

func char to_lower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

func int main() {
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
func int array_sum(int[5] arr) {
    int sum = 0;
    for (int i = 0; i < 5; i++) {
        sum += arr[i];
    }
    return sum;
}

func int main() {
    int[5] numbers = [10, 20, 30, 40, 50];
    int total = array_sum(numbers);
    println("Sum:", total);  // 150
    return 0;
}
```

### 配列の最大値・最小値

```cb
func int find_max(int[10] arr) {
    int max = arr[0];
    for (int i = 1; i < 10; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    return max;
}

func int find_min(int[10] arr) {
    int min = arr[0];
    for (int i = 1; i < 10; i++) {
        if (arr[i] < min) {
            min = arr[i];
        }
    }
    return min;
}

func int main() {
    int[10] data = [45, 12, 89, 23, 67, 34, 91, 56, 78, 10];
    
    println("Max:", find_max(data));  // 91
    println("Min:", find_min(data));  // 10
    
    return 0;
}
```

### 配列の反転

```cb
func void reverse_array(int[5]& arr) {
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

func int main() {
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
func void bubble_sort(int[10]& arr) {
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

func int main() {
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
func int binary_search(int[10] arr, int target) {
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

func int main() {
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
func int fibonacci_iterative(int n) {
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

func int main() {
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

func void stack_init(Stack& s) {
    s.top = -1;
}

func bool stack_is_empty(Stack& s) {
    return s.top == -1;
}

func bool stack_is_full(Stack& s) {
    return s.top == 99;
}

func void stack_push(Stack& s, int value) {
    if (!stack_is_full(s)) {
        s.top++;
        s.data[s.top] = value;
    }
}

func int stack_pop(Stack& s) {
    if (!stack_is_empty(s)) {
        int value = s.data[s.top];
        s.top--;
        return value;
    }
    return -1;
}

func int main() {
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

func void queue_init(Queue& q) {
    q.front = 0;
    q.rear = -1;
    q.size = 0;
}

func bool queue_is_empty(Queue& q) {
    return q.size == 0;
}

func bool queue_is_full(Queue& q) {
    return q.size == 100;
}

func void queue_enqueue(Queue& q, int value) {
    if (!queue_is_full(q)) {
        q.rear = (q.rear + 1) % 100;
        q.data[q.rear] = value;
        q.size++;
    }
}

func int queue_dequeue(Queue& q) {
    if (!queue_is_empty(q)) {
        int value = q.data[q.front];
        q.front = (q.front + 1) % 100;
        q.size--;
        return value;
    }
    return -1;
}

func int main() {
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

---

## 6. ポインタ活用

### スワップ関数

```cb
func void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

func int main() {
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
func int sum_with_pointer(int* ptr, int size) {
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sum += *(ptr + i);
    }
    return sum;
}

func int main() {
    int[5] arr = [10, 20, 30, 40, 50];
    int* ptr = &arr[0];
    
    int total = sum_with_pointer(ptr, 5);
    println("Sum:", total);  // 150
    
    return 0;
}
```

### コールバック関数

```cb
func int apply_operation(int(*operation)(int, int), int a, int b) {
    return operation(a, b);
}

func int add(int a, int b) {
    return a + b;
}

func int multiply(int a, int b) {
    return a * b;
}

func int main() {
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
func int calculate(char op, int a, int b) {
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

func int main() {
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

func int calculate_total(Student& s) {
    int total = 0;
    for (int i = 0; i < 3; i++) {
        total += s.scores[i];
    }
    return total;
}

func double calculate_average(Student& s) {
    int total = calculate_total(s);
    return total / 3.0;
}

func int main() {
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
func void countdown(int seconds) {
    for (int i = seconds; i > 0; i--) {
        println(i);
        // 実際の待機は実装環境による
    }
    println("Time's up!");
}

func int main() {
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

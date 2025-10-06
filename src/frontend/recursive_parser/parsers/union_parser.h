#ifndef UNION_PARSER_H
#define UNION_PARSER_H

class RecursiveParser;
struct UnionDefinition;

class UnionParser {
public:
    explicit UnionParser(RecursiveParser* parser);
    
    bool parseUnionValue(UnionDefinition& union_def);
    
private:
    RecursiveParser* parser_;
};

#endif // UNION_PARSER_H

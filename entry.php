<?php declare(strict_types=1);


namespace App\ToolsModule\CaseGenerator;


use Nette\Utils\Strings;

class Entry
{

    const CASE_CAMEL = 'c';
    const CASE_PASCAL = 'P';
    const CASE_SNAKE = 's';
    const CASE_SCREAMING_SNAKE = 'S';
    const CASE_KEBAB = 'k';


    /**
     * @var array<int,string>
     */
    private array $words;


    public function __construct(array $words)
    {
        $this->words = $words;
    }


    public static function fromString(string $input): ?static
    {
        $input = Strings::toAscii($input);
        $input = Strings::trim($input);
        $re = <<<'EOF'
/
(?'all'
(?>[A-Z\d][a-z\d]+)
|
(?>[A-Z\d]+(?=([A-Z\d][a-z\d])|($)))
|
(?>[A-Z\d]+)
|
(?>[a-z\d]+)
)
/x
EOF;
        preg_match_all($re, $input, $matches, PREG_SET_ORDER, 0);

        $words = [];
        foreach ($matches as $match) {
            $words[] = strtolower($match['all']);


        }
        if (count($words)) {
            return new self($words);
        }

        return null;
    }


    public function toCase(TokenDial $case): string
    {
        $r = '';
        switch ($case) {
            case TokenDial::CamelCase:
                {
                    foreach ($this->words as $word) {
                        $r .= Strings::firstUpper($word);
                        $r = Strings::firstLower($r);
                    }
                }
                break;
            case TokenDial::PascalCase:
                {
                    foreach ($this->words as $word) {
                        $r .= Strings::firstUpper($word);
                    }
                }
                break;
            case TokenDial::SnakeCase:
                {
                    $r = implode('_', $this->words);
                }
                break;
            case TokenDial::ScreamingSnakeCase:
                {
                    $r = implode('_', $this->words);
                    $r = strtoupper($r);
                }
                break;
            case TokenDial::KebabCase:
                {
                    $r = implode('-', $this->words);
                }
                break;


        }

        return $r;
    }


}

<?php declare(strict_types=1);


namespace App\ToolsModule\CaseGenerator;


class Compiler
{

    public string $recipe;

    /**
     * @var array<int,string|TokenDial>
     */
    public array $recipeTokenized = [];

    /**
     * @var array<int,string|TokenDial|array<int,string|TokenDial>>
     */
    public array $recipeParsed = [];

    public int $loopCount = 0;

    /**
     * @var array<int,array>
     */
    public array $generatedParts = [];

    public string $entriesString;


    /**
     * @var Entry[]
     */
    public array $entryList = [];


    public function __construct(string $recipe, string $entriesString)
    {
        $this->recipe = $recipe;
        $this->entriesString = $entriesString;
    }


    public function run(): void
    {
        $this->createEntryList();
        $this->tokenizer();
        $this->parser();
        $this->generator();

    }


    private function createEntryList(): void
    {
        $lineList = preg_split('/\r\n|\r|\n/', $this->entriesString);
        foreach ($lineList as $line) {
            $entry = Entry::fromString($line);
            if ($entry) {
                $this->entryList[] = $entry;
            }
        }
    }


    private function tokenizer(): void
    {

        $re = /* @lang RegExp */
            <<<'EOF'
            /
            (?>%(?'token'.))
            |
            (?'string'[^%]+)
            /x
EOF;

        preg_match_all(
            $re,
            $this->recipe,
            $matches,
            PREG_SET_ORDER | PREG_UNMATCHED_AS_NULL
        );

        foreach ($matches as $match) {
            $token = $match['token'];
            if (!is_null($token)) {
                $value = TokenDial::tryFrom((string)$token);
                if (is_null($value)) {
                    throw new Sorry("Unrecognised token %" . $token);
                }


            } elseif (!is_null($match['string'])

            ) {
                $value = $match['string'];
            } else {
                throw new Sorry('Somewhat unmatched nothing');

            }
            $this->recipeTokenized[] = $value;
        }
    }


    private function parser(): void
    {
        //assume loopless version
        $insideLoop = true;

        //make loopfull if proven
        foreach ($this->recipeTokenized as $item) {
            if ($item === TokenDial::LoopStart) {
                $insideLoop = false;
                break;
            }
        }
        if ($insideLoop) {
            $this->loopCount++;
            $this->recipeParsed[] = [];

        }


        foreach ($this->recipeTokenized as $item) {

            if ($item === TokenDial::LoopStart) {
                if ($insideLoop) {
                    throw new Sorry('Cannot open new loop inside loop');
                }
                $this->loopCount++;
                $insideLoop = true;
                $this->recipeParsed[] = [];
            } elseif ($item === TokenDial::LoopEnd) {
                if (!$insideLoop) {
                    throw new Sorry('Unexpected end of loop');

                }
                $insideLoop = false;
            } elseif (!$insideLoop && in_array($item, TokenDial::IN_LOOP_ONLY)) {
                throw new Sorry('Token %' . $item->value . ' is not allowed outside loop');

            } else {
                if ($insideLoop) {
                    $this->recipeParsed[array_key_last($this->recipeParsed)][] = $item;
                } else {
                    $this->recipeParsed[] = $item;
                }
            }


        }
    }


    private function generator(): void
    {
        foreach ($this->recipeParsed as $recipeStep) {
            if (is_array($recipeStep)) {
                foreach ($this->entryList as $entry) {
                    foreach ($recipeStep as $item) {
                        if (in_array($item, TokenDial::CASES)) {
                            $this->generatedParts[] = [$item, $entry];
                        } else {
                            $this->generatedParts[] = [$item];
                        }

                    }

                }

            } else {
                $this->generatedParts[] = [$recipeStep];
            }


        }

    }


    public function plain(): string
    {
        $r = '';

        foreach ($this->generatedParts as $part) {
            $partContent = $part[0];
            if (is_string($partContent)) {
                $r .= $partContent;
            } else {
                if (in_array($partContent, TokenDial::CASES)) {
                    /** @var Entry $entry */
                    $entry = $part[1];

                    $r .= $entry->toCase($partContent);
                } else {
                    $r .= match ($partContent) {
                        TokenDial::Percent => "%",
                        TokenDial::Tab => "\t",
                        TokenDial::FakeTab => "    ",
                        TokenDial::Newline => "\n",
                    };
                }

            }

        };

        return $r;
    }


    public function pretty(): string
    {
        $r = '';

        foreach ($this->generatedParts as $part) {
            $partContent = $part[0];
            if (is_string($partContent)) {
                for ($i = 0; strlen(@$partContent[$i]); $i++ ) {
                    $char = @$partContent[$i];
                    $r .= match ($char) {
                        "<" => '<span class="cgt cgt-escaped">&lt;</span>',
                        ">" => '<span class="cgt cgt-escaped">&gt;</span>',
                        "&" => '<span class="cgt cgt-escaped">&amp;</span>',
                        "$" => '<span class="cgt cgt-escaped">&#36;</span>',
                        '\'','"','@','.','/','\\', ';' => '<span class="cgt cgt-escaped">'.$char.'</span>',
                        "\n" => '<span class="cgt cgt-newline"><br></span>',
                        "\t" => '<span class="cgt cgt-tab">' . "\t" . '</span>',
                        " " => '<span class="cgt cgt-space"> </span>',
                        default => $char,

                    };
                }


            } else {
                if (in_array($partContent, TokenDial::CASES)) {
                    /** @var Entry $entry */
                    $entry = $part[1];

                    $case = $entry->toCase($partContent);
                    $r .= '<span class="cgt cgt-case">' .$case . '</span>';
                } else {
                    $r .= match ($partContent) {
                        TokenDial::Percent => '<span class="cgt cgt-percent">' . "%" . '</span>',
                        TokenDial::Tab => '<span class="cgt cgt-tab">' . "\t" . '</span>',
                        TokenDial::FakeTab => '<span class="cgt cgt-fake-tab">    </span>',
                        TokenDial::Newline => '<span class="cgt cgt-newline"><br></span>',
                    };
                }

            }

        }

        return $r;
    }

}

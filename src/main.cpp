#include <iostream>
#include <vector>
#include <fstream>
#include <termios.h>
#include <unistd.h>
#include <string>

using namespace std;

// Klasa Tile
// Definicja typu wyliczeniowego 
// `Wall = 0`, `Free = 1`, `Goal = 2`, `Box = 3`, `Player = 4`, ...
enum TileType { Wall, Free, Goal, Box, Player, PlayerOnGoal, BoxOnGoal };

class Tile {
    public:
        TileType type;
        Tile(TileType t) : type(t) {} // Konstruktor
};

// Klasa Board reprezentująca planszę gry
class Board {
    private:    
        vector<vector<Tile>> grid;  // Dwuwymiarowa tablica reprezentująca planszę
        int playerX, playerY;       // Pozycje gracza na planszy

    public:
        // Ładowanie poziomu z pliku
        void loadLevel(const string &filename, int levelNumber) {
            ifstream file(filename);
            if (!file.is_open()) {
                cerr << "Nie można otworzyć pliku: " << filename << endl;
                return;
            }
        
            grid.clear();
            string line;
            bool isCurrentLevel = false;
            int maxRowLength = 0; // Maksymalna długość wiersza dla danego poziomu
        
            vector<vector<Tile>> tempGrid; // Tymczasowa plansza
        
            while (getline(file, line)) {
                // Ignoruj komentarze i sekcje inne niż poziomy
                if (line.size() > 0 && line[0] == ';' && !(line.size() >= 6 && line.compare(0, 6, ";LEVEL") == 0)) {
                    continue;
                }
        
                // Sprawdź, czy linia oznacza początek poziomu
                if (line.size() >= 6 && line.compare(0, 6, ";LEVEL") == 0) {
                    int currentLevel = stoi(line.substr(7)); // Pobierz numer poziomu
                    isCurrentLevel = (currentLevel == levelNumber);
                    continue;
                }
        
                // Jeśli napotkano separator "---", zakończ odczyt bieżącego poziomu
                if (line == "---") {
                    isCurrentLevel = false;
                    continue;
                }
        
                // Jeśli jesteśmy w odpowiednim poziomie, odczytaj jego dane
                if (isCurrentLevel && !line.empty() && line[0] != ';') {
                    vector<Tile> row;
                    for (int x = 0; x < line.size(); ++x) {
                        char c = line[x];
                        switch (c) {
                            case '#': 
                                row.push_back(Tile(Wall)); 
                                break;
                            case ' ': 
                                row.push_back(Tile(Free)); 
                                break;
                            case '.': 
                                row.push_back(Tile(Goal)); 
                                break;
                            case '$': 
                                row.push_back(Tile(Box)); 
                                break;
                            case '@': 
                                row.push_back(Tile(Player)); 
                                playerX = x; 
                                playerY = tempGrid.size(); // Pozycja gracza
                                break;
                            default: 
                                row.push_back(Tile(Free)); 
                                break;
                        }
                    }
                    maxRowLength = max(maxRowLength, static_cast<int>(row.size())); // Aktualizuj maksymalną długość wiersza
                    tempGrid.push_back(row);
                }
            }
        
            file.close();
        
            // Uzupełnij krótsze wiersze pustymi polami (`Free`)
            for (auto &row : tempGrid) {
                while (row.size() < maxRowLength) {
                    row.push_back(Tile(Free));
                }
            }
        
            grid = tempGrid; // Przenieś tymczasową planszę do właściwej
        }
        
        
        
        

        // Rysowanie planszy w konsoli
        void drawBoard() {
            for (int y = 0; y < grid.size(); ++y) {
                for (int x = 0; x < grid[y].size(); ++x) {
                    switch (grid[y][x].type){
                        case Wall: cout << "#"; break;
                        case Free: cout << " "; break;
                        case Goal: cout << "."; break;
                        case Box: cout << "$"; break;
                        case Player: cout << "@"; break;
                        case PlayerOnGoal: cout << "!"; break; // Gracz na celu
                        case BoxOnGoal: cout << "&"; break;   // Skrzynia na celu
                    }
                }
                cout << endl;
            }
        }

        bool isMoveValid(int x, int y) {
            // Sprawdź, czy pole jest poza granicami planszy
            if (x < 0 || y < 0 || y >= grid.size() || x >= grid[0].size()) {
                return false;
            }
        
            // Sprawdź, czy pole jest ścianą lub skrzynią
            TileType tile = grid[y][x].type;
            if (tile == Wall || tile == Box || tile == BoxOnGoal) {
            // if (tile == Wall) {
                return false;
            }
        
            // Pole jest dostępne
            return true;
        }

        bool canPushBox(int boxX, int boxY, int dx, int dy) {
            int newX = boxX + dx; // Nowa pozycja skrzyni
            int newY = boxY + dy;

            // Sprawdź, czy nowe pole dla skrzyni jest poza granicami planszy
            if (newX < 0 || newY < 0 || newY >= grid.size() || newX >= grid[0].size()) {
                return false;
            }

            // Sprawdź, czy nowe pole dla skrzyni jest dostępne (Free lub Goal)
            TileType tile = grid[newY][newX].type;
            if (tile == Free || tile == Goal) {
                return true;
            }

            // Pole jest niedostępne
            return false;
        }

        bool isGameWon() {
            for (int y = 0; y < grid.size(); ++y) {
                for (int x = 0; x < grid[y].size(); ++x) {
                    // Sprawdź, czy istnieje pole celu, które nie jest zajęte przez skrzynię
                    if (grid[y][x].type == Goal || grid[y][x].type == PlayerOnGoal) {
                        return false; // Pole celu nie jest zajęte przez skrzynię - gra jeszcze trwa
                    }
                }
            }
            return true; // Wszystkie pola celu są zajęte przez skrzynie
        }
        
        
        

        // Przesuwanie gracza
        void movePlayer(int dx, int dy) {
            int newX = playerX + dx; // Nowa pozycja gracza
            int newY = playerY + dy;
        
            // Sprawdź, czy gracz może wejść na nowe pole
            if (!isMoveValid(newX, newY)) {
                // Jeśli pole jest zajęte przez skrzynię, sprawdź możliwość jej przesunięcia
                if (grid[newY][newX].type == Box || grid[newY][newX].type == BoxOnGoal) {
                    int boxNewX = newX + dx; // Nowa pozycja skrzyni
                    int boxNewY = newY + dy;
        
                    // Sprawdź, czy skrzynia może zostać przesunięta
                    if (canPushBox(newX, newY, dx, dy)) {
                        moveBox(newX, newY, dx, dy); // Przesuń skrzynię
                    } else {
                        return; // Nie można przesunąć skrzyni - ruch niedozwolony
                    }
                } else {
                    return; // Pole niedostępne - ruch niedozwolony
                }
            }
        
            // Aktualizuj pozycję gracza
            if (grid[playerY][playerX].type == PlayerOnGoal) {
                grid[playerY][playerX] = Tile(Goal); // Gracz opuszcza cel
            } else {
                grid[playerY][playerX] = Tile(Free); // Gracz opuszcza puste miejsce
            }
        
            if (grid[newY][newX].type == Goal) {
                grid[newY][newX] = Tile(PlayerOnGoal); // Gracz wchodzi na cel
            } else {
                grid[newY][newX] = Tile(Player); // Gracz wchodzi na zwykłe pole
            }
        
            playerX = newX;
            playerY = newY;
            
        }
            

        // Przesuwanie skrzyni
        void moveBox(int boxX, int boxY, int dx, int dy) {
            int newX = boxX + dx; // Nowa pozycja skrzyni
            int newY = boxY + dy;
        
            // Aktualizuj pozycję skrzyni
            if (grid[boxY][boxX].type == BoxOnGoal) {
                grid[boxY][boxX] = Tile(Goal); // Skrzynia opuszcza cel
            } else {
                grid[boxY][boxX] = Tile(Free); // Skrzynia opuszcza puste miejsce
            }
        
            if (grid[newY][newX].type == Goal) {
                grid[newY][newX] = Tile(BoxOnGoal); // Skrzynia wchodzi na cel
            } else {
                grid[newY][newX] = Tile(Box); // Skrzynia wchodzi na zwykłe pole
            }
        }
};

// Klasa Game
class Game{
    private:
        Board board;
        int currentLevel; // Numer aktualnego poziomu
        int maxLevels;    // Maksymalna liczba poziomów

    public:
        Game() : currentLevel(1), maxLevels(88) {} // Domyślnie zaczynamy od poziomu 1, zakładam 88 poziomw

        void initializeGame(const string &filename, int levelNumber) {
            currentLevel = levelNumber; // Ustaw aktualny poziom
            board.loadLevel(filename, levelNumber);
        }
    
        // Funkcja do przełączenia terminala w tryb raw mode
        void enableRawMode() {
            termios term;
            tcgetattr(STDIN_FILENO, &term); // Pobierz aktualne ustawienia terminala
            term.c_lflag &= ~(ICANON | ECHO); // Wyłącz tryb kanoniczny i echo
            tcsetattr(STDIN_FILENO, TCSANOW, &term); // Zastosuj zmiany natychmiast
        }

        // Funkcja do przywrócenia normalnego trybu terminala
        void disableRawMode() {
            termios term;
            tcgetattr(STDIN_FILENO, &term);
            term.c_lflag |= (ICANON | ECHO); // Włącz tryb kanoniczny i echo
            tcsetattr(STDIN_FILENO, TCSANOW, &term);
        }

        // Funkcja do odczytu strzałek
        char getArrowKey() {
            char c;
            read(STDIN_FILENO, &c, 1); // Odczytaj pierwszy znak

            if (c == '\033') { // Jeśli to sekwencja escape (kod strzałki zaczyna się od \033)
                char seq[2];
                read(STDIN_FILENO, &seq[0], 1); // Odczytaj '['
                read(STDIN_FILENO, &seq[1], 1); // Odczytaj kod kierunku

                if (seq[0] == '[') {
                    switch (seq[1]) {
                        case 'A': return 'w'; // Strzałka w górę -> 'w'
                        case 'B': return 's'; // Strzałka w dół -> 's'
                        case 'C': return 'd'; // Strzałka w prawo -> 'd'
                        case 'D': return 'a'; // Strzałka w lewo -> 'a'
                    }
                }
            }
            return c; // Zwróć inny znak (np. q do wyjścia)
        }

        void processInput(const string &filename) {
            char input = getArrowKey(); // Odczytaj strzałkę
    
            switch (input) {
                case 'w': board.movePlayer(0, -1); break; // Ruch w górę
                case 's': board.movePlayer(0, 1); break;  // Ruch w dół
                case 'a': board.movePlayer(-1, 0); break; // Ruch w lewo
                case 'd': board.movePlayer(1, 0); break;  // Ruch w prawo
                case 'q': 
                    cout << "Wyjście z gry!" << endl;
                    exit(0); 
                    break;
                default: 
                    cout << "Nieznany ruch!" << endl; 
                    break;
            }

            // Sprawdź warunek zakończenia gry po każdym ruchu
            checkWinCondition(filename);
        }

        void checkWinCondition(const string &filename) {
            if (board.isGameWon()) {
                cout << "Gratulacje! Ukończyłeś poziom " << currentLevel << "!" << endl;
        
                if (currentLevel < maxLevels) {
                    ++currentLevel; // Przejdź do następnego poziomu
                    board.loadLevel(filename, currentLevel); // Załaduj kolejny poziom
                    cout << "Załadowano poziom " << currentLevel << "." << endl;
                } else {
                    cout << "Gratulacje! Ukończyłeś wszystkie poziomy!" << endl;
                    exit(0); // Zakończ program po ukończeniu wszystkich poziomów
                }
            }
        }
        
        // Funkcja wyświetlająca menu i umożliwiająca wybór planszy
        int displayMenu() {
            cout << "========== MENU ==========" << endl;
            cout << "1. Poziom 1" << endl;
            cout << "2. Poziom 2" << endl;
            cout << "3. Poziom 3" << endl;
            cout << "....." << endl;
            cout << "88. Poziom 88" << endl;
            cout << "==========================" << endl;
            cout << "Wybierz poziom (1-88): ";
        
            int choice;
            cin >> choice;
        
            if (choice < 0 || choice > 88) {
                cout << "Nieprawidłowy wybór! Domyślnie ładowany poziom 1." << endl;
                return 1; // Domyślnie poziom 1
            }
        
            return choice; // Zwróć numer wybranego poziomu
        }

        void draw() {
            board.drawBoard();
        }

};

// Funkcja main
int main() {
    Game game;

    // Wyświetlenie menu i wybór numeru poziomu
    int levelNumber = game.displayMenu();

    game.enableRawMode(); // Włącz tryb raw mode terminala

    // Inicjalizacja gry z wybranego pliku poziomu
    game.initializeGame("levels.txt", levelNumber);

    char input;
    while (true) {
        // Wyświetlenie planszy
        game.draw();

        // Obsługa ruchu gracza
        game.processInput("levels.txt");

        // Opcjonalnie: warunek zakończenia gry (np. wszystkie skrzynie na celach)
        // Na razie brak implementacji warunku wygranej.
    }

    game.disableRawMode(); // Przywróć normalny tryb terminala przed zakończeniem programu

    return 0;
}

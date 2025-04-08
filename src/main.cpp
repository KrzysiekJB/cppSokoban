#include <iostream>
#include <vector>
#include <fstream>
#include <termios.h>
#include <unistd.h>
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
        void loadLevel(const string &filename){
            ifstream file(filename);
            if (!file.is_open()){
                cerr << "Nie mona otwoyć pliku: " << filename << endl;
                return;
            }

            grid.clear();
            string line;
            int y = 0;
            while (getline(file, line)){
                vector<Tile> row;
                for (int x = 0; x < line.size(); ++x){
                    char c = line[x];
                    switch (c) {
                        case '#': 
                            row.push_back(Tile(Wall)); 
                            break;
                        case '.': 
                            row.push_back(Tile(Free)); 
                            break;
                        case 'G': 
                            row.push_back(Tile(Goal)); 
                            break;
                        case 'B': 
                            row.push_back(Tile(Box)); 
                            break;
                        case 'P': 
                            row.push_back(Tile(Player)); 
                            playerX = x; 
                            playerY = y; 
                            break;
                        case 'O': // Gracz na polu celu
                            row.push_back(Tile(PlayerOnGoal));
                            playerX = x; 
                            playerY = y; 
                            break;
                        case 'X': // Skrzynia na polu celu
                            row.push_back(Tile(BoxOnGoal)); 
                            break;
                        default: 
                            row.push_back(Tile(Free)); 
                            break;
                    }
                }
                grid.push_back(row);
                ++y;
            }            
        }

        // Rysowanie planszy w konsoli
        void drawBoard() {
            for (int y = 0; y < grid.size(); ++y) {
                for (int x = 0; x < grid[y].size(); ++x) {
                    switch (grid[y][x].type){
                        case Wall: cout << "#"; break;
                        case Free: cout << "."; break;
                        case Goal: cout << "G"; break;
                        case Box: cout << "B"; break;
                        case Player: cout << "P"; break;
                        case PlayerOnGoal: cout << "O"; break; // Gracz na celu
                        case BoxOnGoal: cout << "X"; break;   // Skrzynia na celu
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
                    if (grid[y][x].type == Goal) {
                        return false; // Pole celu nie jest zajęte - gra jeszcze trwa
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

            // Sprawdź warunek zakończenia gry
            if (isGameWon()) {
                cout << "Gratulacje! Ukończyłeś poziom!" << endl;
                exit(0); // Zakończ program
            }
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

    public:
        void initializeGame(const string &levelFile) {
            board.loadLevel(levelFile);
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

        void processInput() {
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
        }

        void draw() {
            board.drawBoard();
        }

        // Funkcja wyświetlająca menu i umożliwiająca wybór planszy
    string displayMenu() {
        cout << "========== MENU ==========" << endl;
        cout << "1. Poziom 1 (level1.txt)" << endl;
        cout << "2. Poziom 2 (level2.txt)" << endl;
        cout << "3. Poziom 3 (level3.txt)" << endl;
        cout << "==========================" << endl;
        cout << "Wybierz poziom (1-3): ";

        int choice;
        cin >> choice;

        // Mapowanie wyboru na nazwę pliku
        switch (choice) {
            case 1: return "../levels/level1.txt";
            case 2: return "../levels/level2.txt";
            case 3: return "../levels/level3.txt";
            default:
                cout << "Nieprawidłowy wybór! Domyślnie ładowany poziom 1." << endl;
                return "../levels/level1.txt";
        }
    }

};

// Funkcja main
int main() {
    Game game;

    // Wyświetlenie menu i wybór planszy
    string levelFile = game.displayMenu();

    game.enableRawMode(); // Włącz tryb raw mode terminala

    // Inicjalizacja gry z wybranego pliku poziomu
    game.initializeGame(levelFile);

    char input;
    while (true) {
        // Wyświetlenie planszy
        game.draw();

        // Obsługa ruchu gracza
        game.processInput();

        // Opcjonalnie: warunek zakończenia gry (np. wszystkie skrzynie na celach)
        // Na razie brak implementacji warunku wygranej.
    }

    game.disableRawMode(); // Przywróć normalny tryb terminala przed zakończeniem programu

    return 0;
}

import { Component } from '@angular/core';
import { Header } from './components/header/header';
import { Footer } from './components/footer/footer';
import { Sidebar } from './components/sidebar/sidebar';
import { Cards } from './components/cards/cards';
import { Hobby } from './models/hobby';

@Component({
  selector: 'app-root',
  imports: [Header, Footer, Sidebar, Cards],
  templateUrl: './app.html',
  styleUrl: './app.css',
})
export class App {
  hobbies: Hobby[] = [
    { id: 1, title: 'Piano', description: 'Playing piano in my free time.', icon: '🎹', favorite: true },
    { id: 2, title: 'Ping Pong', description: 'Fast-paced games with friends.', icon: '🏓', favorite: true },
    { id: 3, title: 'Basketball', description: 'Shooting hoops and playing pickup games.', icon: '🏀', favorite: true },
    { id: 4, title: 'Reading', description: 'Getting lost in a good book.', icon: '📚', favorite: false },
    { id: 5, title: 'Cooking', description: 'Trying out new recipes.', icon: '🍳', favorite: false },
    { id: 6, title: 'Developing', description: 'Building personal projects.', icon: '💻', favorite: true },
  ];
}
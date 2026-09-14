import { Component } from '@angular/core';
import { CardComponent } from './card/card';
import { PlanetModel } from './planet.model';

@Component({
  selector: 'app-root',
  standalone: true,
  imports: [CardComponent],
  templateUrl: './app.html',
  styleUrl: './app.css'
})
export class App {
  title = 'Space Network Communication System';

  planetsList: PlanetModel[] = [
    {
      name: 'Mars',
      distance: 225,
      imageUrl: 'mars.jpg',
      satellites: 3
    },
    {
      name: 'Jupiter',
      distance: 778,
      imageUrl: 'jupiter.jpg',
      satellites: 5
    },
    {
      name: 'Saturn',
      distance: 1400,
      imageUrl: 'saturn.jpg',
      satellites: 2
    },
    {
      name: 'Venus',
      distance: 108,
      imageUrl: 'venus.jpg',
      satellites: 1
    }
  ];
}
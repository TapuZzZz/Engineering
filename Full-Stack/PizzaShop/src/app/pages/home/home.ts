import { Component } from '@angular/core';
import { RouterLink } from '@angular/router';

@Component({
  selector: 'app-home',
  imports: [RouterLink],
  templateUrl: './home.html',
  styleUrl: './home.css'
})
export class Home {
  features = [
    { icon: '🔥', title: 'Stone oven', text: 'Baked at 400°C for a crispy crust' },
    { icon: '🍅', title: 'Fresh ingredients', text: 'Local vegetables, every morning' },
    { icon: '🛵', title: 'Fast delivery', text: 'At your door in 30 minutes' }
  ];
}
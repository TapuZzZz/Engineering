import { Component, Input } from '@angular/core';
import { PlanetModel } from '../planet.model';

@Component({
  selector: 'app-card',
  standalone: true,
  imports: [],
  templateUrl: './card.html',
  styleUrl: './card.css'
})
export class CardComponent {
  @Input("planet") planet: PlanetModel = new PlanetModel();
}